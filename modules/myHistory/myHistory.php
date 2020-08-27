<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 * @since			Sep 10, 2012
 */


class myHistory_ctrl extends Controller
{

	public function sql2json() {
		$params = $this->post;

		echo \utils::jsonForTabletop($this->db, PREFIX . 'versions', $params);
	}

	public function show_all()
	{
		$fields = ['id', 'user', 'time', 'tb', 'rowid', 'content', 'editsql', 'editvalues'];
		
		$this->render('myHistory', 'read', [
			'th_fields' => '<th>' . implode('</th><th>', $fields) . '</th>',
			'm_data' => '{"mData":"' . implode('"},{"mData":"', $fields) . '"}',
			'ajaxSource' => "./?obj=myHistory_ctrl&method=sql2json"
		]);
	}

}
