<?php
/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 * @since			May 2, 2018
 */

interface iPreProcess
{
	/**
	 * Runs middlevare
	 * @param  array $get  array of GET data
	 * @param  array $post  array of POST data
	 * @return array 		[halt: boolean, echo: string, headers: array]
	 */
	public function run($get = false, $post = false);


}
