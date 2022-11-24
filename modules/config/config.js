/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 */

var config = {

	init: function(){

		api.confirmSuperAdmPwd( d => {
			core.open({
				title: core.tr('sys_config'),
				obj: 'config_ctrl',
				method: 'home',
			});
		});

	},

	validateApp: (uid) => {
		core.getHTML('config_ctrl', 'validate_app', false, false, html => {
			$('#' + uid + ' .edit-column').html(html);
			$('#' + uid +' .field-list-column').html('').removeClass('col-sm-3');
		})
	},

	viewAppProperties: (uid) => {
		core.getHTML('config_ctrl', 'app_properties', false, false, html => {
			$('#' + uid + ' .edit-column').html(html);
			$('#' + uid +' .field-list-column').html('').removeClass('col-sm-3');
		})
	},

	viewTbProperties: function(tb, uid){
		core.getHTML('config_ctrl', 'table_properties', tb ? {tb:tb} : false, false, html => {
			$('#' + uid + ' .edit-column').html(html);
			$('#' + uid + ' .field-list-column').html('').removeClass('col-sm-3');
		})
	},

	viewFldList: function(tb, uid){
		core.getHTML('config_ctrl', 'fld_list', { tb: tb }, false, html => {
			$('#' + uid + ' .field-list-column').html(html);
			$('#' + uid + ' .field-list-column').addClass('col-sm-3');
			$('#' + uid + ' .edit-column').html('');
		})
	},

	viewFldProperties: function(tb, fld, thisel){
		const toEl = $(thisel).parents('.main').children('.edit-column');
		let get = { tb: tb };
		if (fld) { get.fld = fld; }
		core.getHTML('config_ctrl', 'field_properties', get, false, html => {
			toEl.html(html);
		})
	},

	saveTbData: function(form, tb){
		core.getJSON('config_ctrl', 'save_tb_data', false, form.serializeArray(), function(data){
			  core.message(data.text, data.status);
			  if (data.status === 'success'){
				config.viewTbProperties(tb, $('div.active .config_container').attr('id') );
			  }
		  });
	},

	addNewTb: function(form){
		core.getJSON('config_ctrl', 'add_new_tb', false, form.serializeArray(), function(data){
			  core.message(data.text, data.status);
			  if (data.status === 'success'){
				layout.tabs.reloadActive();
				setTimeout(()=> {
					config.viewTbProperties(data.tb, $('div.active .config_container').attr('id') );
				}, 500);
			  }
		  });
	},

	saveFldProperties: (form, tb, fld ) => {
		core.getJSON('config_ctrl', 'save_fld_properties', false, form.serializeArray(), function(data){
			core.message(data.text, data.status);
			if (data.status === 'success'){
				config.viewFldProperties(tb, fld, form);
			}
		});
	},

	addNewFld: (form, tb ) => {
		core.getJSON('config_ctrl', 'add_new_fld', false, form.serializeArray(), function(data){
			core.message(data.text, data.status);
			if (data.status === 'success'){
				config.viewFldProperties(tb, data.fld, form);
				config.viewFldList(tb, form.parents('.config_container').attr('id'))
			}
		});
	},

	saveAppProperties: (form) => {
		core.getJSON('config_ctrl', 'save_app_properties', false, form.serializeArray(), function(data){
			core.message(data.text, data.status);
		});
	},

	deleteTb: tb => {
		core.open({
			title: core.tr('delete_table'),
			html: '<p class="lead">' + core.tr('warning_delete_table') + '</p>',
			buttons: [
				{
					text: core.tr('close'),
					action: 'close'
				},
				{
					addclass: 'btn-danger',
					text: core.tr('delete_table'),
					click: function(){
						core.getJSON('config_ctrl', 'delete_tb', { tb: tb }, false, data => {
							$('#modal').modal('hide');
							core.message(data.text, data.status);
							layout.tabs.reloadActive();
						})
					}
				}
			]
		}, 'modal');
	},

	deleteColumn: (tb, fld, clickedButton) => {
		core.open({
			title: core.tr('delete_column'),
			html: '<p class="lead">' + core.tr('warning_delete_column') + '</p>',
			buttons: [
				{
					text: core.tr('close'),
					action: 'close'
				},
				{
					addclass: 'btn-danger',
					text: core.tr('delete_column'),
					click: function(){
						core.getJSON('config_ctrl', 'delete_column', { tb: tb, fld: fld }, false, data => {
							$('#modal').modal('hide');
							core.message(data.text, data.status);
							config.viewFldList(tb, $(clickedButton).parents('.config_container').attr('id'));
						})
					}
				}
			]
		}, 'modal');
	},
	
	renameTb: (oldName) => {
		core.open({
			title: core.tr('rename_table'),
			html: `<p class="lead text-danger">
				${core.tr('warning_rename_table')}
			</p>
			<hr>
			<input type="text" class="form-control new-tb-name" value="${oldName}">`,
			buttons: [
				{
					text: core.tr('close'),
					action: 'close'
				},
				{
					addclass: 'btn-danger',
					text: core.tr('rename_table'),
					click: function(){
						const newName = $('#modal input.new-tb-name').val();
						core.getJSON('config_ctrl', 'rename_tb', { old_name: oldName, new_name: newName }, false, data => {
							$('#modal').modal('hide');
							core.message(data.text, data.status);
							if (data.status === 'success'){
								layout.tabs.reloadActive();
							}
						})
					}
				}
			]
		}, 'modal');
	},

	renameFld: (tb, oldName, clickedButton) => {
		core.open({
			title: core.tr('rename_column'),
			html: `<p class="lead text-danger">${core.tr('warning_rename_column')}</p><hr><input type="text" class="form-control new-column-name" value="${oldName}">`,
			buttons: [
				{
					text: core.tr('close'),
					action: 'close'
				},
				{
					addclass: 'btn-danger',
					text: core.tr('rename_column'),
					click: function(){
						const newName = $('#modal input.new-column-name').val();
						core.getJSON('config_ctrl', 'rename_column', { tb: tb, old_name: oldName, new_name: newName }, false, data => {
							$('#modal').modal('hide');
							core.message(data.text, data.status);
							if (data.status === 'success'){
								$('#modal').modal('hide');
								core.message(data.text, data.status);
								config.viewFldList(tb, $(clickedButton).parents('.config_container').attr('id'));
							}
						})
					}
				}
			]
		}, 'modal');
	},
	fix: (thisButton, action, tb, col) => {
		core.runAndRespond('config_ctrl', 'fix', {action: action, tb: tb, col: (col ? col : null )}, false, d => {
			if (d.status === 'success'){
				$(thisButton).parent().remove()
			}
		});
	},

	setFldOptions: (tb, dest_select) => {
		core.getJSON('config_ctrl', 'getFldList', {tb: tb}, false, d => {
			const fields = d.fields;
			$(dest_select).find('option').remove();
			for (const [key, value] of Object.entries(fields)) {
				$(dest_select).append(`<option value="${key}">${value}</option>`)
			}
		})
	},

	viewGeoFaceProperties: (uid) => {
		core.getHTML('config_ctrl', 'geoface_properties', false, false, html => {
			$('#' + uid + ' .edit-column').html(html);
			$('#' + uid +' .field-list-column').html('').removeClass('col-sm-3');
		})
	},
	saveGeoFaceProperties: (form, uid) => {
		core.getJSON('config_ctrl', 'save_geoface_properties', false, form.serializeArray(), function(data){
			core.message(data.text, data.status);
			if(data.status === 'success'){
				config.viewGeoFaceProperties(form.parents('.config_container').attr('id'))
			}
		});
	},

	deleteGeoFile: (file, clickedButton) => {
		core.open({
			'title': core.tr('delete') + ' ' + file,
			'html': core.tr('confirm_delete_geofile', [file]),
			'buttons': [
				{
					'text': 'Delete',
					'addclass': 'btn btn-danger',
					'click': (dialog) => {
						core.getJSON('config_ctrl', 'delete_local_geofile', {file: file}, false, function(data){
							core.message(data.text, data.status);
							dialog.modal('hide');
							if(data.status === 'success'){
								config.viewGeoFaceProperties(clickedButton.parents('.config_container').attr('id'))
							}
						})
					}
				},
				{
					'text': core.tr('cancel'),
					'click': 'close'
				}
			]
		}, 'modal');
	},
};