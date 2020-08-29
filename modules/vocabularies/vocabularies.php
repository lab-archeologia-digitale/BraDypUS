<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 * @since			Aug 10, 2012
 */

 use \DB\System\Manage;

class vocabularies_ctrl extends Controller
{
	private $sys_manager = false;

	private function getSysMng()
	{
		if (!$this->sys_manager){
			$this->sys_manager = new Manage($this->db, $this->prefix);
		}
		return $this->sys_manager;
	}

	private function getFullVocabularies()
	{
		$res = $this->getSysMng()->getBySQL('vocabularies', '1=1 ORDER BY voc, sort');
		$vocs = [];

		foreach($res as $arr) {
			$vocs[$arr['voc']][$arr['id']] = $arr['def'];
		}
		return $vocs;
	}

	public function show()
	{
		$this->render('vocabularies', 'list', [
			'vocs' => $this->getFullVocabularies(),
		]);
	}
	
	public function add_new_form()
	{
		$voc = $this->get['voc'] ?: false ;

		if (!$voc) {
			$all_vocs = array_keys($this->getFullVocabularies());
		}
		
		$this->render('vocabularies', 'new_form', [
			'voc' => $voc,
			'all_vocs' => $all_vocs,
		]);
	}
	
	public function edit()
	{
		$id = $this->get['id'];
		$val = $this->get['val'];

		$res = $this->getSysMng()->editRow('vocabularies', $id, [
			'def' => $val
		]);
		
		if ( $res ) {
			utils::response('ok_def_update', 'success');
		} else {
			utils::response('error_def_update', 'error');
		}
	}
	
	public function erase()
	{
		$id = $this->get['id'];

		$res = $this->getSysMng()->deleteRow('vocabularies', $id );

		if ( $res ) {
			utils::response('ok_def_erase', 'success');
		} else {
			utils::response('error_def_erase', 'error');
		}
	}
	
	public function add()
	{
		$voc = $this->get['voc'];
		$def = $this->get['def'];

		$res = $this->getSysMng()->addRow('vocabularies', [
			'voc' => $voc,
			'def' => $def
		]);

		if ( $res ) {
			utils::response('ok_def_added', 'success');
		} else {
			utils::response('error_def_added', 'error');
		}
	}
	
	public function sort()
	{
		$error = false;
		foreach($this->post as $voc => $sort_data) {
			foreach ($sort_data as $sort => $id) {
				$res = $this->getSysMng()->editRow('vocabularies', (int)$id, [
					'sort' => $sort
				]);
				if (!$res){
					$error = true;
				}
			}
		}

		if ($error){
			utils::response('error_sort_update', 'error');
		} else {
			utils::response('ok_sort_update', 'success');
		}
	}
}