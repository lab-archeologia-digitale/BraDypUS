/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 */

var tbs_editor = {
		init: function(){
			
			core.open({
				title: core.tr('edit_tbs_data'),
				obj: 'tbs_editor_ctrl',
				method: 'tb_list',
			});
		},
		
		saveData: function(form, tb){
			core.getJSON('tbs_editor_ctrl', 'save', false, form.serializeArray(), function(data){
				  core.message(data.text, data.status);
				  if (data.status == 'success'){
					  tbs_editor.loadTbForm(form.parent('.form_container'), tb);
				  }
			  });
		},
		
		loadTbForm: function(form_container, tb){
			form_container.html(core.loading).load('./?obj=tbs_editor_ctrl&method=show_form&tb=' + tb, function(){
				
				$(this).find('button.add_new_fld').click(function(){
					flds_editor.loadform($(this).data('table'), false, form_container);
				});

				$(this).find('button.openform').click(function(){
					flds_editor.loadform($(this).data('table'), $(this).data('field'), form_container);
				});
			});
		},
};