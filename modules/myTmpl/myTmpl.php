<?php
/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 * @since			Aug 11, 2012
 */

class myTmpl_ctrl extends Controller
{
	public function show()
	{
		$tbs = $this->cfg->get('tables.*.label', 'is_plugin', null);

		$tmpls = \utils::dirContent(PROJ_DIR . 'templates/') ?: [];

		$data = [];

		foreach ($tbs as $tb=>$label) {
			foreach ($tmpls as $tmpl) {
				if (preg_match('/' . str_replace($this->prefix, '', $tb) . '/', $tmpl)) {
					$data[$tb]['list'][] = $tmpl;
				}
			}
			$data[$tb]['default_read'] = $this->cfg->get("tables.$tb.tmpl_read");
			$data[$tb]['default_edit'] = $this->cfg->get("tables.$tb.tmpl_edit");
			$data[$tb]['user_read'] = \pref::getTmpl($tb, 'read');
			$data[$tb]['user_edit'] = \pref::getTmpl($tb, 'edit');
		}

		$this->render('myTmpl', 'show', [
			'tabs' => $tbs,
			'data' => $data,
		]);

	}

	/**
	 * Saves user preferences to session (pref)
	 * @param string $tb
	 * @param string $context
	 * @param string $tmpl template name with extension
	 */
	public function changeTmpl()
	{
		$tb = $this->get['tb'];
		$context = $this->get['context'];
		$tmpl = $this->get['tmpl'];

		\pref::setTmpl($tb, $context, $tmpl);
		
		$this->response('ok_tmpl_set', 'success');
	}
}
