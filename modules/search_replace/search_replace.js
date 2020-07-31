/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 */

var search_replace = {
		init: function(){
			core.open({
				obj: 'search_replace_ctrl',
				method: 'main_page',
				title: core.tr('find_replace'),
				buttons:[
				         {
				        	 text: core.tr('find_replace'),
				        	 click: function(){
				        		 search_replace.run();
				        	 }
				         },
				         {
				        	 text: core.tr('cancel'),
				        	 action: 'close'
				         }
				         ]
				
			},
			'modal');
			
			return;
		},
		
		run: function(){
			var tb = $('#modal select.tb').val(),
				TB = $('#modal select.tb option:selected').text(),
				fld = $('#modal select.fld').val(),
				FLD = $('#modal select.fld option:selected').text(),
				search = $('#modal input.search').val(),
				replace = $('#modal input.replace').val();
		 
			if (!tb || !fld || !search || !replace){
				core.message(core.tr('all_fields_required'), 'error');
			} else {
				core.open({
					title: core.tr('attention'),
					html: core.tr('confirm_search_replace', [TB, FLD, search, replace]),
					buttons:[
					         {
					        	 text: core.tr('confirm'),
			 		        	 click: function(){
			 		        		 $.get('./?&obj=search_replace_ctrl&method=replace&tb=' + tb + '&fld=' + fld + '&search=' + search + '&replace=' + replace, function(data){
			 		        			 if (data == 'error'){
			 		        				 core.message(core.tr('error_search_replace'), 'error');
			 		        			 } else {
			 		        				core.message(core.tr('ok_search_replace', [data]), 'success');
			 		        				$('#modal').modal('hide');
			 		        			 }
			 		        		 });
			 		        	 } 
					         },
					         {
					        	 text: core.tr('cancel'),
					        	 action: 'close'
					         }
					         ]
				}, 'modal');
				
				
			}
		},
		
		setFld: function(uid, tb){
			
			// show div
			$('#' + uid).find('div.fld').show();
			
			//remove all options
			$('#' + uid).find('select.fld').html($('<option />'));
			
			//get field names
			$.get('./?obj=search_replace_ctrl&method=getFld&tb=' + tb, function(data){
				$.each(data, function(index, el){
					$('<option />')
						.attr('value', index)
						.text(el)
						.appendTo($('#' + uid).find('select.fld'));
				});
			}, 'json');
		}
};
