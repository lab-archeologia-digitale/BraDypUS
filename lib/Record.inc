<?php
/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 * @since			Apr 21, 2012
 * @uses			cfg
 *
 * $data['core'] = array of core data [field_id=>value]
 * $data['plugins'] = array of arrays of plugin data [plugin_name=>[fld_id=>value]]
 * $data['corelinks_details'] = array of arrays links id and id_field: [table_name => [ [id=int, id_field=string], [...]], [...] ]
 * $data['corelinks'] = array with sum-up info about corelinks [table_name => [tot:int, query:string, values:array], [...]]
 * $data['userlinks'] =
 * $data['files'] =
 * 
 * $data	=>	[
 * 		"core"		=> [
 * 				fieldid => fieldvalue,
 * 				fieldid => fieldvalue,
 * 				...
 * 		],
 * 		"plugins"	=> [
 * 				plugin1_name => [
 * 					fieldid => fieldvalue,
 * 					fieldid => fieldvalue,
 * 					...
* 				],
 * 				plugin2_name => [
 * 					...
* 				],
				...
 * 		],
 * 		"corelinks_details": [
 * 				
 * 		],
 * ]
 */

use DB\DBInterface;
use \Config\Config;

class Record implements RecordInterface
{
	protected $tb;
	protected $id;
	protected $db;
	protected $cfg;
	protected $data;
	protected $loaded;
	protected $changed;
	protected $idField;
	protected $tmp_dir;

	public function __construct($tb, $id = false, DBInterface $db, Config $cfg)
	{
		$this->tb = $tb;
		$this->db = $db;
		$this->cfg = $cfg;
		$this->tmp_dir = sys_get_temp_dir() . '/';

		if ($id === 'last_added') {
			$t = $db->query('SELECT id FROM ' . $tb . ' WHERE 1=1 ORDER BY id DESC LIMIT 1 OFFSET 0', null, 'read');
			$id = $t[0]['id'];
		}
		$this->id = $id;
	}

	public function getId()
	{
		return $this->id;
	}

	/**
	 * Returns multi-dimensional array with all available data for record:
	 * 		core
	 * 		coreLinks
	 * 		allPlugins
	 * 		fullFiles
	 * 		rs
	 * 		userLinks
	 *
	 * @return array
	 */
	public function readFull()
	{
		$rec = [];

		$rec['core'] 		= $this->getCore();

		$rec['coreLinks']	= $this->getCoreLinks();

		$rec['allPlugins']	= $this->getAllPlugins(true);

		$rec['fullFiles']	= $this->getFullFiles();

		if ($this->cfg->get("tables.{$this->tb}.rs")) {
			$rec['rs'] 		= $this->getRS();
		}

		$rec['userLinks'] 	= $this->getUserLinks();

		return $rec;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getTb()
	 */
	public function getTb() : string
	{
		return $this->tb;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getDB()
	 */
	public function getDB() : DBInterface
	{
		return $this->db;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::persist()
	 */
	public function persist()
	{
		if (is_array($this->changed['core']) AND count($this->changed['core']) > 0) {

			// CORE.update
			if ($this->id AND !empty($this->id)) {

				foreach ($this->data['core'] as $key => $val) {

					if ($key !== 'id' AND in_array($key, $this->changed['core'])) {

						$tmp[] = "{$key} = ?";
						$values[] = $val;
					}
				}

				$query = 'UPDATE ' . $this->tb . ' SET ' . implode(', ', $tmp) . ' WHERE id = ? ';
				$values[] = $this->id;

				// Backup record:
				$this->db->backupBeforeEdit($this->tb, $this->id, $query, $values);
				$flag['core'] = $this->db->query($query, $values, 'boolean');

				// moves updaded file
				if ($this->tb === PREFIX . 'files') {

					if (file_exists($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'])) {

						@copy(
							$this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'],
							PROJ_DIR . 'files/' . $this->data['core']['id'] . '.' . $this->data['core']['ext']
						);
					}
				}

			} else {

				// CORE.insert
				foreach ($this->data['core'] as $key => $val) {
					$values[":{$key}"] = $val;
				}

				$values[':creator'] = $_SESSION['user']['id'];
				$query = "INSERT INTO {$this->tb} (creator, " . 
					implode(", ", array_keys($this->data['core'])) . ") VALUES (:creator, :" . implode(', :', array_keys($this->data['core'])). ")";

				$last_id = $this->db->query($query, $values, 'id');

				// moves updaded file
				if ($this->tb === PREFIX . 'files') {

					if (file_exists($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'])) {

						@copy($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'], PROJ_DIR . 'files/' . $last_id . '.' . $this->data['core']['ext']);
					}
				}


				$flag['core'] = $last_id;

				$this->data['core']['id'] = $last_id;

				$this->id = $last_id;
			}
		}

		// RS
		if ($this->changed['rs']) {

			$q_arr = array();

			foreach ($this->data['rs'] as $rel) {

				if (!$rel['id']) {

					array_push($q_arr, array(
							'q' => 'INSERT INTO ' . PREFIX . 'rs (tb, first, second, relation) VALUES (:tb, :first, :second, :relation)',
							'values'=>array(':tb'=>$this->tb, ':first'=>$rel['first'], ':second'=>$rel['second'], ':relation'=>$rel['relation'])
							));
				}
			}
			$flag['rs'] = $this->query2db($q_arr);
		}


		//PLUGINS
		if (is_array($this->data['plugins'])) {

			$query = [];

			foreach ($this->data['plugins'] as $plg_name => $plg_all_rows) {

				foreach ($plg_all_rows as $plg_row) {

					$tmp = [];
					$values = [];

					$has_value_flag = false;

					foreach ($plg_row as $key => $val) {

						if ($key !== 'table_link' AND $key !== 'id_link' AND !empty($val)) {
							$has_value_flag = true;
						}

						if ($key !== 'id' ) {
							$tmp[] = "{$key} = :{$key}";
							$values[":{$key}"] = $val;
						}
					}

					// plg.row.DELETE
					if($plg_row['id_delete'] && !empty($plg_row['id_delete'])) {

						$delete_plg_query = 'DELETE FROM ' . $plg_name . ' WHERE id = ?';
						$delete_plg_values = [ $plg_row['id_delete'] ];
						
						// Backup before delete
						$this->db->backupBeforeEdit($plg_name, $plg_row['id_delete'], $delete_plg_query, $delete_plg_values);
						array_push($query, array(
								'q' => $delete_plg_query,
								'values' => $delete_plg_values
						));

					} else if($plg_row['id'] && !empty($plg_row['id'])) {

						// plg.row.UPDATE
						$update_plg_query = 'UPDATE ' . $plg_name . ' SET ' . implode(', ', $tmp) . ' WHERE id = :idwhere';
						$values[':idwhere'] = $plg_row['id'];

						// Backup before update
						$this->db->backupBeforeEdit($plg_name, $plg_row['id'], $update_plg_query, $values);

						array_push($query, array(
								'q' => $update_plg_query,
								'values' => $values
						));

					} else {
						// plg.row.INSERT
						if ($has_value_flag) {
							// if context is add new must re-set the value of core_id manually!
							if (empty($plg_row['id_link']) AND $this->data['core']['id']) {
								$plg_row['id_link'] = $this->data['core']['id'];
								$values[':id_link'] = $this->data['core']['id'];
							}
							unset($plg_row['id']);

							array_push($query, array(
									'q' => 'INSERT INTO ' . $plg_name . " (" . implode(", ", array_keys($plg_row)) . ") VALUES (:" . implode(', :', array_keys($plg_row)). ")",
									'values' => $values
							));
						}
					}
				}
			}

			if (is_array($query)) {

				$this->db->beginTransaction();

				foreach ($query as $q_arr) {
					$ret = $this->db->query($q_arr['q'], $q_arr['values'], 'boolean');
					$flag['plugins'][($ret ? 'success' : 'error')][] = true;
				}

				$this->db->commit();
			}
		}

		return $flag;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::delete()
	 */
	public function delete()
	{
		$id = $this->id ? $this->id : $this->data['core']['id'];

		if (!$id) {
			return false;
		}

		// CORE
		$q[] = 'DELETE FROM ' . $this->tb . ' WHERE id = ' . $id;
		// Backup before delete
		$this->db->backupBeforeEdit($this->tb, $id, 'DELETE FROM ' . $this->tb . ' WHERE id = ' . $id);

		// PLUGINS
		$plugins = $this->cfg->get("tables.{$this->tb}.plugin");

		if (is_array($plugins)) {

			foreach ($plugins as $plg) {
				// TODO: NO BACKUP HERE
				$q[] = 'DELETE FROM ' . $plg . ' WHERE table_link = \'' . $this->tb. '\' AND id_link = ' . $id;
			}
		}

		// USERLINKS
		// TODO: NO BACKUP HERE
		$q[] = 'DELETE FROM ' . PREFIX . 'userlinks WHERE '
			." ( tb_one = '" . $this->tb . "' AND id_one = '" . $id . "')"
			. ' OR '
			. "( tb_two = '" . $this->tb . "' AND id_two = '" . $id . "')";

		//DELETE FILE (deleted only if all db erases are OK)
		// TODO: NO BACKUP HERE
		if ($this->tb == PREFIX . 'files') {
			$file_to_delete = PROJ_DIR . 'files/' . $id . '.' . $this->getCore('ext');
		}

		// RS
		// TODO: NO BACKUP HERE
		$rs_fld = $this->cfg->get("tables.{$this->tb}.rs");
		if ($rs_fld) {

			if (!$this->loaded['core']) {
				$this->getCore();
			}

			$q[] = 'DELETE FROM ' . PREFIX . 'rs WHERE ' . "tb = '" . $this->tb . "' AND ( "
						. " first =  '" . $this->data['core'][$rs_fld] . "' "
						. ' OR '
						. " second =  '" . $this->data['core'][$rs_fld] . "' "
					. ' )';

		}


		$this->db->beginTransaction();

		try {

			foreach ($q as $query) {
				$this->db->query($query);
			}

			$this->db->commit();

			if ($file_to_delete and file_exists($file_to_delete)) {
				@unlink($file_to_delete);
			}

			return true;

		} catch (\DB\DBException $e) {
			$this->db->rollBack();
			throw new \Exception($e);
		}
	}

	/**
	 * Sets idField value
	 * @param string $idField
	 */
	public function setIdField($idField)
	{
		$this->idField = $idField;
	}

	/**
	 *
	 * Loads core data
	 */
	private function loadCore()
	{
		if ($this->id) {

			$query = "SELECT * FROM " . $this->tb . " WHERE id = :id";
			$values = array(':id' => $this->id);

		} else if ($this->idField) {

			$query = "SELECT * FROM " . $this->tb . " WHERE " . $this->cfg->get("tables.{$this->tb}.id_field"). " = :id_field LIMIT 1 OFFSET 0";
			$values = array(':id_field' => $this->idField);
		}

		if ($query && $values) {
			$res = $this->db->query($query, $values, 'read');
		}

		$this->loaded['core'] = true;

		if (is_array($res[0])) {
			$this->data['core'] = $res['0'];
		}
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::setCore()
	 */
	public function setCore( array $core_data) : void
	{
		if (!$this->loaded['core']) {
			$this->loadCore();
		}

		if (!is_array($this->data['core'])) {
			// it means that no id was found => it's a new record
			$this->data['core'] = [];
		}

		if (is_array($core_data[$this->tb])) {

			foreach ($core_data[$this->tb] as $key => $value) {
				$this->data['core'][$key] = is_array($value) ? implode('; ', $value) : $value;
				$this->changed['core'][] = $key;
			}
		}

		$this->loaded['core'] = true;
	}


	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getCore()
	 */
	public function getCore( string $fieldname = null)
	{
		if (!$this->loaded['core']) {
			$this->loadCore();
		}

		if ($fieldname) {
			return $this->data['core'][$fieldname];
		} else {
			return $this->data['core'];
		}
	}

	/**
	 *
	 * Loads specifica plugin data
	 * @param string $plugin plugin name
	 * @param int	$core_id id of main table
	 */
	private function loadPlugin($plugin, $core_id = false)
	{
		$core_id = $core_id ? $core_id : $this->data['core']['id'];

		if (!$core_id) {
			$this->loaded['plugins'][$plugin] = true;
			return;
		}

		$query = "SELECT * FROM " . $plugin . " WHERE table_link = :table_link AND id_link = :id_link";

		$res = $this->db->query($query, array(':table_link'=>$this->tb, ':id_link' => $core_id));

		$this->loaded['plugins'][$plugin] = true;

		if (count($res) > 0) {
			// Sort plugins using sort field, if available
			if (in_array('sort', array_keys($res[0]))) {
				usort($res, function($a, $b){
					if ($a['sort'] == $b['sort']) {
							return 0;
					}
					return ($a['sort'] > $b['sort']) ? 1 : -1;
				});
			}
			$this->data['plugins'][$plugin] = $res;
		}
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::setPlugin()
	 */
	public function setPlugin( string $plugin, array $plugin_data) : void
	{
		if ($this->id) {
			$this->loadAllPlugins();
		}

		foreach ($plugin_data as $row_id => &$row) {
			$row['table_link'] = $this->tb;
			$row['id_link'] = $this->id;
		}

		if (!is_array($this->data['plugins'][$plugin])) {
			$this->data['plugins'][$plugin] = [];
		}

		foreach ($plugin_data as $key => &$data_arr) {
			foreach ($data_arr as $k => &$v) {
				if (is_array($v)) {
					$v = implode(';', $v);
				}
			}
		}

		$this->data['plugins'][$plugin] = array_merge($this->data['plugins'][$plugin], $plugin_data);

		$this->changed['plugins'][$plugin] = true;

		$this->loaded['plugins'][$plugin] = true;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getPlugin()
	 */
	public function getPlugin(string $plugin, int $index = null, string $fieldname = null)
	{
		$this->getCore();

		if (!$this->loaded['plugins'][$plugin]) {
			$this->loadPlugin($plugin);
		}

		if ($index !== false AND $fieldname) {
			return $this->data['plugins'][$plugin][$index][$fieldname];
		} else {
			return $this->data['plugins'][$plugin];
		}
	}

	/**
	 *
	 * Loads all plugin data
	 * @param int	$core_id id of main table
	 */
	private function loadAllPlugins()
	{
		$plugins = $this->cfg->get("tables.{$this->tb}.plugin");

		if (!$plugins) {
			$this->loaded['allPlugins'] = true;
			return;
		}

		foreach ($plugins as $plugin) {
			$this->loadPlugin($plugin, $this->id);
		}
		$this->loaded['allPlugins'] = true;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getAllPlugins()
	 */
	public function getAllPlugins( bool $load = false) : array
	{
		$this->getCore();

		if ($load || !$this->loaded['allPlugins']) {
			$this->loadAllPlugins();
		}

		return $this->data['plugins'];
	}

	/**
	 * Loads link data
	 * @param boolean $detail
	 */
	private function loadCoreLinks($detail = false)
	{
		$links = $this->cfg->get("tables.{$this->tb}.link");

		if (!$links) {
			$this->loaded['corelinks'] = true;
			return;
		}

		if (!$this->loaded['core']) {
			$this->loadCore();
		}

		foreach ($links as $link) {
			/*
			 * $link = array('other_tb' => string, 'fld'=>array( 0 => array('other': string, 'my'=>string))
			 * if !is_array(fld)	:	error
			 * if !is_array(fld[0])	:	error
			 */

			if ( !is_array($link['fld']) || !is_array($link['fld'][0]) ) {
				error_log('Configuration problem. Tables > ' . $this->tb . ' > links > ' . $link['other_tb'] . ' NOT well formatted and will be ignored');
				continue;
			}

			$tmp = [];
			$tmp_values = [];

			foreach ($link['fld'] as $fld_arr) {
				$tmp[] = ' ' . $fld_arr['other'] . " = ? ";
				$tmp_values[] = $this->data['core'][$fld_arr['my']] ?? '';
			}

			$id_fld = $this->cfg->get("tables.{$link['other_tb']}.id_field");

			$query = "SELECT id " . ( ($id_fld === 'id') ? '' : ', ' . $id_fld . '') . " FROM " . $link['other_tb'] . " WHERE " . implode(' AND ', $tmp);
			$res = $this->db->query($query, $tmp_values, 'read');

			if ($res) {

				if (!$detail) {
					$this->data['corelinks'][$link['other_tb']]['tot'] = count($res);
					$this->data['corelinks'][$link['other_tb']]['query'] = implode(' AND ', $tmp);
					$this->data['corelinks'][$link['other_tb']]['values'] = $tmp_values;

				} else {
					foreach ($res as $row) {
						$this->data['corelinks_details'][$link['other_tb']][] = $row;
					}
				}
			}
		}
		$detail ?  ($this->loaded['corelinks_details'] = true) : ($this->loaded['corelinks'] = true);
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getBackLinks()
	 */
	public function getBackLinks()
	{
		if ($this->data['backLink']) {
			return $this->data['backLink'];
		}

		$ret = [];

		$bl_array = $this->cfg->get("tables.{$this->tb}.backlinks");

		if (!$bl_array) {
			return $ret;
		}

		foreach($bl_array as $bl_string) {

			list($tb, $plg, $plg_fld) = \utils::csv_explode($bl_string, ':');

			if (!$tb || !$plg || !$plg_fld) {
				error_log('Syntax error in configuration of ' . $this->tb . ' segment backlink');
				continue;
			}

			$this_id_fld_val = $this->getCore($this->cfg->get("tables.{$this->tb}.id_field"));

			if (!$this_id_fld_val) {
				continue;
			}

			$dest_id_fld = $this->cfg->get("tables.$tb.id_field");

			if ($dest_id_fld === 'id') {
				// If id_field of the destrination table is id field, the query is simple
				$sql = "SELECT id_link as id FROM {$plg} WHERE table_link = '{$tb}' AND {$plg_fld} = '" . $this->getCore('id') . "'";

			} else {
        // SQl query is more complicated and is run on the destination table
				$sql = <<<EOD
SELECT id, {$dest_id_fld} as label FROM {$tb}
	WHERE id IN (
		SELECT id_link FROM {$plg} WHERE
			table_link = '{$tb}'
			AND
			{$plg_fld} = '{$this->getCore('id')}'
		)
EOD;
			}

			$r = $this->db->query($sql, null, 'read');
			if ($r[0]['id']) {
				$ret[$tb] = $r;
			}
		}

		$this->data['backLink'] = $ret;

		return $this->data['backLink'];
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getCoreLinks()
	 */
	public function getCoreLinks( bool $details = false) : array
	{
		if ($details) {
			if (!$this->loaded['corelink_details']) {
				$this->loadCoreLinks(true);
			}
			return $this->data['corelinks_details'];
		} else {
			if (!$this->loaded['corelink']) {
				$this->loadCoreLinks();
			}
			return $this->data['corelinks'];
		}
	}

	/**
	 *
	 * Load full data for linked file(s)
	 */
	private function loadFullFiles()
	{
		if (!$this->loaded['core']) {
			$this->loadCore();
		}
		
		if ($this->tb == PREFIX . 'files') {

			$this->data['fullfiles'][0] = $this->data['core'];

		} else {

			if ($this->data['core']['id']) {
				$query = 'SELECT ' . PREFIX . 'files.*, ' . PREFIX . 'userlinks.id as linkid'
						. ' FROM ' . PREFIX . 'files JOIN ' . PREFIX . 'userlinks'
						. ' ON ( '
							. "(" . PREFIX . "userlinks.tb_one = '" . PREFIX . "files' AND  " . PREFIX . "userlinks.id_one = " . PREFIX . "files.id)"
							. ' OR '
							. "(" . PREFIX . "userlinks.tb_two = '" . PREFIX . "files' AND  " . PREFIX . "userlinks.id_two = " . PREFIX . "files.id)"
						. ')'
						. ' WHERE '
							. "(" . PREFIX . "userlinks.tb_one = '" . $this->tb . "' AND  " . PREFIX . "userlinks.id_one = '" . $this->data['core']['id'] . "')"
							. ' OR '
							. "(" . PREFIX . "userlinks.tb_two = '" . $this->tb . "' AND  " . PREFIX . "userlinks.id_two = '" . $this->data['core']['id'] . "')"
						. ' ORDER BY ' . PREFIX . 'userlinks.sort, ' . PREFIX . 'userlinks.id';
				$this->data['fullfiles'] = $this->db->query($query, null, 'read');
			}
		}

		$this->loaded['fullfiles'] = true;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getFullFiles()
	 */
	public function getFullFiles()
	{
		if (!$this->loaded['fullfiles']) {
			$this->loadFullFiles();
		}
		return $this->data['fullfiles'];
	}

	private function loadRS()
	{

		$this->data['rs'] = $this->db->query(
			'SELECT * FROM ' . PREFIX . 'rs WHERE tb = ? AND (first = ? OR second = ?)', 
			[ $this->tb, $this->id, $this->id], 
			'read'
		);

		$this->loaded['rs'] = true;
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::getRS()
	 */
	public function getRS()
	{
		if (!$this->loaded['rs']) {
			$this->loadRS();
		}
		return $this->data['rs'];
	}

	/**
	 * (non-PHPdoc)
	 * @see RecordInterface::setRS()
	 */
	public function addRS($first, $relation, $second)
	{
		$first = trim($first);
		$second = trim($second);
		$res = $this->db->query(
				'SELECT * FROM ' . PREFIX . 'rs WHERE tb = :tb AND ( (first = :first_one AND second = :second_one) OR (first = :second_two AND second = :first_two) )',
				[
					':tb'=>$this->tb, 
					':first_one'=>$first, 
					':second_one'=>$second,
					':second_two'=>$second,
					':first_two'=>$first
				],
				'read'
		);

		if (count($res) > 0) {
			return false;
		} else {
			return $this->db->query(
					'INSERT INTO ' . PREFIX . 'rs (tb, first, second, relation) VALUES (:tb, :first, :second, :relation)',
					array(':tb'=>$this->tb, ':first'=>$first, ':second'=>$second, ':relation'=>$relation),
					'id'
			);
		}
	}


	public function deleteRS($id)
	{
		return $this->db->query('DELETE FROM ' . PREFIX . 'rs WHERE id = ?', [ $id ], 'boolean');
	}

	/**
	 * Executes all queries of a query array (q=> query text, values=>array with values to bind)
	 * and returns array with result count flag (array(success => array(), error=>array())
	 * @param array $query_arr
	 */
	private function query2db($q_arr)
	{
		$flag = array();

		if (is_array($q_arr)) {
			$this->db->beginTransaction();

			foreach ($q_arr as $q) {
				$ret = $this->db->query($q['q'], $q['values'], 'boolean');
				$flag[($ret ? 'success' : 'error')][] = true;
			}
			$this->db->commit();
		}
		return $flag;
	}


	public function getUserLinks()
	{
		if (!$this->loaded['userlinks']) {
			$this->loadUserLinks();
		}

		return $this->data['userlinks'];
	}

	private function loadUserLinks()
	{
		$userlinks = $this->db->query(
			'SELECT * FROM ' . PREFIX . 'userlinks WHERE (tb_one = ? AND id_one = ?) OR (tb_two = ? AND id_two = ?) ORDER BY sort, id',
			[ 
				$this->tb, 
				$this->id, 
				$this->tb, 
				$this->id 
			],
			'read'
		);

		if ($userlinks) {

			foreach ($userlinks as $userlink) {
				if ($this->tb == $userlink['tb_one'] AND $this->id == $userlink['id_one']) {
					$ul[] = array('id'=>$userlink['id'], 'tb'=>$userlink['tb_two'], 'ref_id'=>$userlink['id_two']);
				} else if ($this->tb == $userlink['tb_two'] AND $this->id == $userlink['id_two']) {
					$ul[] = array('id'=>$userlink['id'], 'tb'=>$userlink['tb_one'], 'ref_id'=>$userlink['id_one']);
				}
			}
		}
		$this->data['userlinks'] = $ul;

		$this->loaded['userlinks'] = true;
	}

	public function deleteUserLink($id)
	{
		return $this->db->query('DELETE FROM ' . PREFIX . 'userlinks WHERE id = ?', [ $id ], 'boolean');
	}

	public function addUserLink($tb, $id)
	{
		return $this->db->query(
				'INSERT INTO ' . PREFIX . 'userlinks (tb_one, id_one, tb_two, id_two) VALUES (:thistb, :thisid, :tb, :id)',
				[ ':thistb'=>$this->tb, ':thisid'=>$this->id, ':tb' => $tb, ':id' => $id ],
				'boolean'
				);
	}
}
