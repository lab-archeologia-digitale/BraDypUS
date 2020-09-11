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
				        		  core.getJSON('backup_ctrl', 'doBackup', false, false, data => {
									  core.message(data.text, data.status);
				        			  if(data.status === 'success'){
										  core.getHTML('backup_ctrl','getSavedBups', false, false, d => {
											$('#modal .modal-body').html(d);
										  });
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
			// TODO:brackets
			core.getJSON('backup_ctrl', 'deleteBackup', {file: file}, false, function(data){
				core.message(data.text, data.status);
				$('#modal .modal-body').load('./?obj=backup_ctrl&method=getSavedBups');
			});
		},
		
		download: function(file){
			window.open(file);
		}
};

