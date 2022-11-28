<?php
/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 * @since				Jul 02, 2018
 *
 *  inspect:
 *      curl --location --request GET '/api/v2/paths?verb=inspect&tb=manuscripts(&pretty=1)'

 *  getChart:
 *      curl --location --request GET '/api/v2/paths?verb=getChart&id=1(&pretty=1)'

 *  read
 *      curl --location --request GET '/api/v2/paths?verb=read&tb=manuscripts&id=1(&pretty=1)'

 *  getVocabulary
 *      curl --location --request GET '/api/v2/paths?verb=getVocabulary&voc=abbreviations(&pretty=1)'
 * 
 *  getUniqueVal
 *      curl --location --request GET '/api/v2/paths?verb=getUniqueVal&tb=manuscripts&fld=cmclid(&s=MICH&w=cmclid|like|%25AA%25&pretty=1)'
 * 
 *  search
 *      curl --location --request GET '/api/v2/paths?verb=search&shortsql=@places(&total_rows=&page=&geojson=&records_per_page&full_records&pretty=1)'
 */

use \DB\System\Manage;
use \API\Inspect;
use \API\GetUniqueVal;
use \API\Search;
use \Record\Read;

class api extends Controller
{
    private $app;
    private $verb;
    private $pretty = false;
    private $valid_verbs = [
        'read',
        'search',
        'inspect',
        'getChart',
        'getUniqueVal',
        'getVocabulary',
        'getApiVersion',
    ];
    /**
     * Main validation & variable setting
     */
    private function validateInput() : void
    {
        $this->pretty = $this->get['pretty'];
        
        // Validate app
        $this->app = $this->get['app'];
        $valid_apps = \utils::dirContent(MAIN_DIR . "projects");
        if (!$this->app || !in_array($this->app, $valid_apps)) {
            throw new \Exception("Invalid app `{$this->app}`. App must be one of " . implode(', ', $valid_apps));
        }

        // Validate verb
        $this->verb = $this->get['verb'];
        if (!$this->verb) {
            throw new \Exception("Missing verb. Verb must be one of " . implode(', ', $this->valid_verbs));
        }
        if (!in_array($this->verb, $this->valid_verbs)) {
            throw new \Exception("Invalid verb `{$this->verb}`. Verb must be one of " . implode(', ', $this->valid_verbs));
        }

        // Tb must have prefix
        if ($this->get['tb'] && strpos($this->get['tb'], $this->prefix) === false) {
            $this->get['tb'] = $this->prefix . $this->get['tb'];
        }

        if ($this->get['tb']) {
            // Validate table
            $sys_manage = new Manage($this->db, $this->prefix);
            if (in_array($this->get['tb'], $sys_manage->available_tables)) {
                throw new \Exception("System tables cannot be queried");
            }
        }
    }

    private function logApiUser() : bool
    {
        $auth_user_id = $this->cfg->get('main.api_login_as_user');
        if (!$auth_user_id){
            return false;
        }
        $sys_manager = new Manage($this->db, $this->prefix);
        $res = $sys_manager->getById('users', (int)$auth_user_id);
        if (!empty($res)){
            \login_ctrl::startUserSession($res);
            return true;
        }
        return false;
    }

    public function run()
    {
        try {
            try {
                $this->logApiUser();
            } catch (\Throwable $th) {
                throw new \Exception('Cannot log user and bootstrap the application');
            }

            if (!\utils::canUser('read')){
                throw new \Exception(\tr::get('not_authorized'));
            }

            $this->validateInput();
            
            $pp_response = $this->preProcess();
            
            if ($pp_response === 'halt') {
                return;
            }

            if (!method_exists($this, $this->verb)) {
                throw new \Exception("Verb {$this->verb} has not been implemented yet");
            }

            $resp = $this->{$this->verb}();
            
            echo $this->array2response($resp);

        } catch (\Throwable $e) {
            echo $this->array2response([
                'type' => 'error',
                'text' => $e->getMessage(),
                'trace' => $this->debug ? $e->getTrace() : "Turn on API debug to read trace"
            ]);
        }
        \login_ctrl::endUserSession();
    }

    private function postProcess($data) : array
    {
        if (file_exists("projects/{$this->get['app']}/mods/api/PostProcess.php")) {
            require_once("projects/{$this->get['app']}/mods/api/PostProcess.php");
            $postProcess = new PostProcess();

            if (!method_exists($postProcess, 'run')) {
                return false;
            }

            $resp = $postProcess->run($data, $this->get, $this->post);

            return [
                'data' => $resp['data'],
                'mime' => $resp['mime']
            ];
        } else {
            return ["data" => $data];
        }
    }

    private function preProcess()
    {
        if (file_exists("projects/{$this->get['app']}/mods/api/PreProcess.php")) {
            require_once("projects/{$this->get['app']}/mods/api/PreProcess.php");
            $preProcess = new PreProcess();

            if (!method_exists($preProcess, 'run')) {
                return false;
            }

            $pp = $preProcess->run($this->get, $this->post);

            if ($pp['headers'] && is_array($pp['headers'])) {
                foreach ($pp['headers'] as $k => $v) {
                    header("$k: $v");
                }
            }
            if ($pp['echo']) {
                echo $pp['echo'];
            }

            if ($pp['halt']) {
                return 'halt';
            }
        } else {
            return $data;
        }
    }

    /**
     * Returns information on table or on all tables structure
     *		$this->get['tb'] is optional
     * @return array
     */
    private function inspect(): array
    {
        $tb = $this->get['tb'];

        $resp = Inspect::Configuration($this->cfg, $tb);
        
        return $resp;
    }

    /**
     * Validates request and returna array of record data
     *		$this->get['tb'] is required
     *		$this->get['id'] is required
     * @return array
     */
    private function read(): array
    {
        $tb = $this->get['tb'];
        $id = $this->get['id'];

        if (!$tb) {
            throw new \Exception("Table name is required with verb read");
        }
        if (!$id) {
            throw new \Exception("Record id is required with verb read");
        }
        $read_record = new Read($id, null, $tb, $this->db, $this->cfg);
        $resp = $read_record->getFull();
        
        return $resp;
    }

    /**
     * Validated request and returns aray of vocabulary items
     *		$this->get['voc'] is requires
     * @return Array
     */
    private function getVocabulary() : array
    {
        $voc = $this->get['voc'];
        if (!$voc) {
            throw new \Exception("Vocabulary name is required with verb getVocabulary");
        }
        
        $sys_manage = new Manage($this->db, $this->prefix);
        $res = $sys_manage->getBySQL('vocabularies', 'voc = ?  ORDER BY sort ASC LIMIT 500 OFFSET 0', [$voc]);
        
        $resp = [];
        if (is_array($res)) {
            foreach ($res as $r) {
                array_push($resp, $r['def']);
            }
        }
        
        return $resp;
    }
    
    /**
     * Validates request and returns array of unique values
     *		$this->get['tb'] is required
     *		$this->get['fld'] is required
     *		$this->get['s'] is optional
     *		$this->get['w'] is optional
     * @return Array
     */
    private function getUniqueVal() : array
    {
        $tb			= $this->get['tb'];
        $fld		= $this->get['fld'];
        $substring	= $this->get['s'];
        $where		= $this->get['w'];

        if (!$tb) {
            throw new \Exception("Table name is required with verb getUniqueVal");
        }
        if (!$fld) {
            throw new \Exception("Field name is required with verb getUniqueVal");
        }

        $resp = GetUniqueVal::run($tb, $fld, $substring, $where, $this->db, $this->cfg, $this->prefix);

        return $resp;
    }
    /**
     * Validates request and returns result of search verb
     * 	$this->get['shortsql'] is required
     * 	$this->get['total_rows'],
     * 		$this->get['page'],
     * 		$this->get['geojson'],
     * 		$this->get['records_per_page'],
     * 		$this->get['full_records'] are optional
     *
     * @return array
     */
    private function search() : array
    {

        $shortsql = $this->get['shortsql'];
        if (!$shortsql) {
            throw new \Exception("ShortSQL text is required with verb search");
        }

        $total_rows 		= $this->get['total_rows']			? (int)$this->get['total_rows']			: false;
        $page 				= $this->get['page']				? (int)$this->get['page']				: false;
        $geojson 			= (bool) $this->get['geojson'];
        $records_per_page	= $this->get['records_per_page'] 	? (int)$this->get['records_per_page']	: false;
        $full_records		= (bool) $this->get['full_records'];
        
        $resp = Search::run(
            $this->prefix,
            $shortsql,
            [
                'total_rows' 		=> $total_rows,
                'page'				=> $page,
                'geojson'			=> $geojson,
                'records_per_page'	=> $records_per_page,
                'full_records'		=> $full_records
            ],
            $this->debug,
            $this->db,
            $this->cfg
        );
        
        return $resp;
    }



    /**
     * Validates request and returns chart data
     *	$this->get['id'] is required. Can be a valid chart id or string 'all'
     * @return array
     */
    private function getChart() : array
    {
        $chartId = (int) $this->get['id'];
        if (!$chartId) {
            throw new \Exception("Chart id is required");
        }
        require_once __DIR__ . '/GetChart.php';
        $resp = GetChart::run($chartId, $this->db, $this->prefix);
        return $resp;
    }

    private function getApiVersion()
    {
        return ['version' => version::current()];
    }

    /**
     * Formats array data as pretty-printed JSON and returns or echoes (with Cross-domain allow policy) it
     * @param  array  $data       array of input data
     * @return string
     */
    private function array2response($data) : string
    {
        $mime = 'application/json';

        if ($data['type'] !== 'error' && $data['records'] ) {
            $pp = $this->postProcess($data);
            $data = $pp['data'];
            $mime ?: $pp['mime'];
        }

        if (is_array($data)) {
            // Always add version to API response
            $flag = $this->pretty ? JSON_PRETTY_PRINT|JSON_UNESCAPED_UNICODE : JSON_UNESCAPED_UNICODE;
            $data = json_encode($data, $flag);
        }

        header('Access-Control-Allow-Origin: *');
        header('Content-type: ' . $mime . '; charset=utf-8');

        return $data;
    }
}
