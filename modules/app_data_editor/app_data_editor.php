<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS 2007-2012
 * @license			All rights reserved
 * @since			Aug 11, 2012
 */

class app_data_editor_ctrl extends Controller
{
	public function getInfo()
	{
		$user = new User(new DB());
		
		$users = $user->getUser('all');
		
		foreach ($users as &$u)
		{
			$u['verbose_privilege'] = utils::privilege($u['privilege'], 1);
		}
		echo $this->render('app_data_editor', 'form', array(
				'available_langs' => utils::dirContent(LOCALE_DIR),
				'info' => cfg::main(),
				'status' => array('on', 'frozen', 'off'),
				'offline' => array(0, 1),
				'tr_disabled' => tr::get('disabled'),
				'users' => $users
				));
		
	}
	
	public function save()
	{
		$data = $this->post;
		
		try
		{
			cfg::setMain($data);
			utils::response('ok_file_updated');
		}
		catch (myException $e)
		{
			utils::response('error_file_updated', 'error');
		}
	}
}