/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 */

var backup = {
		init: function(){
			
			core.open({
				obj: 'backup_ctrl',
				method: 'getSavedBups',
				title: core.tr('backup'),
				buttons: [
				          {
				        	  text: '<i class="glyphicon glyphicon-ok"></i> ' + core.tr('backup_now'),
				        	  click: function(){
				        		  $.get('controller.php?obj=backup_ctrl&method=doBackup', function(data){
				        			  if(!data || data == ''){
				        				  core.message(core.tr('ok_backup'), 'success');
				        				  $('#modal .modal-body').load('controller.php?obj=backup_ctrl&method=getSavedBups');
				        			  } else {
				        				  core.message(core.tr('error_backup') + ' ' + core.tr('details_in_log'), 'error');
				        			  }
				        		  });
				        	  } 
				          },
				          {
				        	  text: '<i class="glyphicon glyphicon-remove"></i> ' + core.tr('close'),
				        	  click: 'close'
				          }
				          ]
			}, 'modal');
		},
		
		erase: function(file, button){
			core.getJSON('backup_ctrl', 'erase', [file], false, function(data){
				core.message(data.text, data.status);
				$('#modal .modal-body').load('controller.php?obj=backup_ctrl&method=getSavedBups');
			});
		},
		
		download: function(file){
			window.open(file);
		}
};

