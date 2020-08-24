<?php 
use \DB\Engines\AvailableEngines;

class config_ctrl extends Controller
{
    public function home()
    {
        $table_list = cfg::tbEl('all', 'label');

        $this->render('config', 'home', [
            "table_list" => $table_list
        ]);
    }

    public function app_properties()
    {
        try{
			$user = new User($this->db);
		
			$users = $user->getUser('all');
		
			foreach ($users as &$u){
				$u['verbose_privilege'] = utils::privilege($u['privilege'], 1);
			}
		} catch (myException $e){
			$users = [];
		}
		
		$this->render('config', 'app_properties', [
            'available_langs' => utils::dirContent(LOCALE_DIR),
            'info' => cfg::main(),
            'status' => [ 'on', 'frozen', 'off' ],
            'users' => $users,
            'db_engines' => \DB\Engines\AvailableEngines::getList()
        ]);
    }

    public function fld_list()
    {
        $tb = $this->get['tb'];

		$this->render('config', 'fld_list', [
            'tb' => $tb,
            'tb_label' => cfg::tbEl($tb, 'label'),
            'all_fields' => cfg::fldEl($tb, 'all', 'all')
        ]);
    }


    public function field_properties()
    {
        $tb = $this->get['tb'];
        $fld = $this->get['fld'] ?: false;

        $data = $fld ? cfg::fldEl($tb, $fld, 'all') : [];

        $voc = new Vocabulary($this->db);
		$all_voc = $voc->getAllVoc();

        $fld_structure = file_get_contents(__DIR__ . '/fld_structure.json');

        $fld_structure = str_replace(
            [
                'list-of-system-defined-vocabularies-here', 
                'list-of-available-tables-here'
            ],
            [
                implode('","', $all_voc), 
                implode('","', array_values(cfg::tbEl('all','name')))
            ],
            $fld_structure
        );
        $fld_structure = json_decode($fld_structure, TRUE);

        $this->render('config', 'field_properties', [
            'tb'    => $tb,
            'fld'   => $fld,
            'data'  => $data,
            'fld_structure' => $fld_structure 
        ]);
    }


    public function table_properties()
    {
        $tb = $this->get['tb'] ?: false;

        $table_properties = $tb ? cfg::tbEl($tb, 'all') : [];

        // default values
		if (!$table_properties['preview'])  $table_properties['preview'] = array(0=>'');
		if (!$table_properties['plugin'])   $table_properties['plugin'] = array(0=>'');
		if (!$table_properties['link'])     $table_properties['link'] = array(0=>array('fld'=>array(0=>'')));

        $this->render('config', 'table_properties', [
            'data'  => $table_properties,
            'tb'    => $tb,
            'field_list' => $tb && cfg::fldEl($tb, 'all', 'label') ? cfg::fldEl($tb, 'all', 'label') : ['id' => 'id'],
            'template_list' => utils::dirContent(PROJ_DIR . 'templates/'),
            'available_plugins' => is_array(cfg::getPlg()) ? cfg::getPlg() : [],
            'available_tables' => cfg::tbEl('all', 'label'),
        ]);

    }

    public function save_tb_data()
	{
		$post = $this->post;

		try {

			$post = utils::recursiveFilter($post);

			// make indexed array for links and geoface
			if($post['link']) {

				$post['link'] = array_values($post['link']);

				$tmp = array_values($post['link']);

				foreach ($tmp as &$link) {
					$link['fld'] = array_values($link['fld']);
				}

				$post['link'] = false;
				$post['link'] = $tmp;
			}

			if ($post['is_plugin'] == 1 && (!$post['name'] || !$post['label'] )) {

				throw new myException('1. Required fields are missing');

			} else if ( (!$post['is_plugin'] || $post['is_plugin'] == 0) && ( !$post['name'] || !$post['label'] || !$post['order'] && !$post['id_field'] || !$post['preview'] ) ) {

				throw new myException('2. Required fields are missing');
            }

			cfg::setTb($post);

			utils::response('ok_cfg_data_updated');

		} catch(myException $e) {
			$this->log->error($e);
			utils::response('error_cfg_data_updated', 'error');
		}
    }


    public function add_new_tb()
	{
		$post = $this->post;

		try {

			$post = utils::recursiveFilter($post);

			if ($post['is_plugin'] == 1 && (!$post['name'] || !$post['label'] )) {

				throw new myException('1. Required fields are missing');

			} else if ( (!$post['is_plugin'] || $post['is_plugin'] == 0) && ( !$post['name'] || !$post['label'] || !$post['order'] && !$post['id_field'] || !$post['preview'] ) ) {

				throw new myException('2. Required fields are missing');
            }

            // Write table columns file
            $new_tb_name = $post['name'];
            cfg::setFld( str_replace(PREFIX, null, $new_tb_name), 'id', [
                "name" => "id",
                "label" => "Id",
                "type" => "text"
            ]);
            cfg::setFld( str_replace(PREFIX, null, $new_tb_name), 'creator', [
                "name" => "creator",
                "label" => "Creator",
                "type" => "text"
            ]);
            
            // Write table data file
            cfg::setTb($post);
            
            // Add table to database
            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->createMinimalTable($new_tb_name);

			utils::response('ok_cfg_data_updated', 'success', false, [
                'tb' => $new_tb_name
            ]);

		} catch(myException $e) {
			$this->log->error($e);
			utils::response('error_cfg_data_updated', 'error');
		}
    }
    

    public function save_fld_properties()
	{
		$post = $this->post;
		try {
			$post = utils::recursiveFilter($post);

			$tb = $post['tb_name'];
			$fld = $post['fld_orig_name'];
			unset($post['tb_name']);
			unset($post['fld_orig_name']);

			if (!$post['name'] || !$post['type']){
				throw new myException('Both field name and field type are required');
            }
            
			cfg::setFld($tb, $fld, $post);

			utils::response('ok_cfg_data_updated');
        
        } catch(myException $e) {
            $this->log->error($e);
			utils::response('error_cfg_data_updated', 'error');
		}
    }

    public function add_new_fld()
	{
		$post = $this->post;
		try {
			$post = utils::recursiveFilter($post);

			$tb = $post['tb_name'];
			$fld = $post['name'];
			unset($post['tb_name']);

			if (!$post['name'] || !$post['type']){
				throw new myException('Both field name and field type are required');
            }
            $available_flds = array_values(cfg::fldEl($tb, 'all', 'name'));
            if (in_array($fld, $available_flds)){
                utils::response(tr::get('fld_already_available', [$fld]), 'error', true);
                return;
            }
            
            cfg::setFld($tb, $fld, $post);
            
            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->addFld($tb, $fld, $post['db_type']);

			utils::response('ok_cfg_data_updated', 'success', false, ["fld" => $fld]);
        
        } catch(myException $e) {
            $this->log->error($e);
			utils::response('error_cfg_data_updated', 'error');
		}
    }
    
    public function save_app_properties()
    {
        $data = $this->post;
		
		try {

			cfg::setMain($data);
            utils::response('ok_cfg_data_updated');
            
		} catch (myException $e) {

			utils::response('error_cfg_data_updated', 'error');
		}
    }

    
    public function delete_tb()
    {
        $tb = $this->get['tb'];
        try {
            cfg::deleteTb($tb);
            // Drop table from database
            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->dropTable($tb);
            utils::response('ok_cfg_tb_delete', 'success');
        } catch (\Throwable $th) {
            utils::response('error_cfg_tb_delete', 'error');
        }
    }


    public function delete_column()
    {
        $tb = $this->get['tb'];
        $fld = $this->get['fld'];

        try {
            cfg::deleteFld($tb, $fld);

            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->dropFld($tb, $fld);

            utils::response('ok_cfg_column_delete', 'success');
        } catch (\Throwable $th) {
            utils::response('error_cfg_clumn_delete', 'error');
        }
    }

    public function security_check_pwd()
    {
        if (!utils::canUser('super_admin')){
            utils::response('not_a_super_admin_user', 'error');
            return;
        }
        // Logged used is super admin. Let's check the password
        $pwd = $this->post['pwd'];
        $current_user_id = $_SESSION['user']['id'];

        $user = new User($this->db);
        
        $me = $user->getUser([
            "id" => $current_user_id,
            "password" => $user->encodePwd($pwd)
        ]);

        if(!$me || !is_array($me)){
            utils::response('invalid_pasword', 'error');
            return;
        } else {
            utils::response('valid_pasword', 'success');
            return;
        }
    }

    public function rename_tb()
    {
        $old_name = $this->get['old_name'];
        $new_name = $this->get['new_name'];
        try {
            $available_tbs = array_values(cfg::tbEl('all', 'name'));
            if (in_array($new_name, $available_tbs)){
                throw new myException("Table name $new_name has already been used");
            }

            cfg::renameTb($old_name, $new_name);

            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->renameTable($old_name, $new_name);

            utils::response('ok_renaming_table', 'success');
        } catch (myException $e) {
            $this->log->error($e);
            utils::response('error_renaming_table', 'error');
        }
    }

    public function rename_column()
    {   
        $tb = $this->get['tb'];
        $old_name = $this->get['old_name'];
        $new_name = $this->get['new_name'];

        try {
            $available_flds = array_values(cfg::fldEl($tb, 'all', 'name'));
            if (in_array($new_name, $available_flds)){
                throw new myException("Field name $new_name has already been used");
            }

            cfg::renameFld($tb, $old_name, $new_name);

            $engine = $this->db->getEngine();
            if ($engine = 'sqlite'){
                $driver = new \DB\Alter\Sqlite($this->db);
            } elseif($engine = 'mysql'){
                $driver = new \DB\Alter\Mysql($this->db);
            } elseif($engine = 'pgsql'){
                $driver = new \DB\Alter\Postgres($this->db);
            } else {
                throw new \Exception("Unknown database engine: `$engine`");
            }
            $alter = new \DB\Alter($driver);
            $alter->renameFld($tb, $old_name, $new_name);

            utils::response('ok_renaming_column', 'success');
        } catch (myException $e) {
            $this->log->error($e);
            utils::response('error_renaming_column', 'error');
        }
    }


    public function validate_app()
    {
        $validate = new \DB\Validate\Validate($this->db, PREFIX);
        $report = $validate->all();

        $html = '<button type="button" class="btn btn-info pull-right" onclick="$(this).parent().find(\'.alert-info, .alert-success\').toggle();">' . tr::get('show_only_errors') . '</button>';

        foreach ($report as $item) {
            if ($item['status'] === 'head'){
                $html .= '<h3>' . $item['text']. '</h3>';
            } else {
                $html .= '<div class="alert alert-' . $item['status'] . '"> ' 
                    . $item['text'] 
                    . ( $item['suggest'] ? '<div>' . $item['suggest'] . '</div>' : '' )
                . '</div>';
            }
        }

        echo $html;

    }
}