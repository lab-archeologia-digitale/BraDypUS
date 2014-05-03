<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS 2007-2011
 * @license			All rights reserved
 * @since			Apr 8, 2012
 */

ob_start();

try
{
	$basePath = './';
	require_once './lib/constants.inc';
	
	$controller = new Controller($_GET, $_POST, $_REQUEST);
	
	$controller->route();
	
}
catch (myException $e)
{
	$e->log();
	echo utils::message( $e->getMessage(), 'error', true);
	
	if ($_SESSION['debug'])
	{
		var_dump($e);
	}
}
catch (Exception $e)
{
	error_log($e->__toString(), 3, ERROR_LOG);
	
	echo utils::message( tr::get('generic_error'), 'error', true);
	
	if ($_SESSION['debug'])
	{
		var_dump($e);
	}	
}
ob_end_flush();