/*
 * copyright BraDypUS 
 */
var rs={init:function(){$.each($('div.showRS'),function(i,el){rs.show($(el));});},show:function(el){if(el.length===0){return;}
var tb=el.data('table'),id=encodeURI(el.data('id')),context=el.data('context');if(!tb||!id||!context){return false;}
if(el.children().length>0){return false;}
if(el.data('pending')===true||el.data('done')===true){return false;}
el.data('pending',true);el.load('./?obj=rs_ctrl&method=getAll&param[]='+tb+'&param[]='+id+'&param[]='+context,function(){el.data('done',true);el.data('pending',false);if(context=='read'){el.find('div.rsEl').on('click',function(){api.record.read(tb,[$(this).text()],true);});}
else if(context=='edit'){el.find('button.save').click(function(){var relation=el.find('select.rel').val(),second=el.find('input.second').val();if(!second){core.message(core.tr('rs_all_fields_required'),'error');}else{core.getJSON('rs_ctrl','saveNew',[tb,id,relation,second],false,function(data){core.message(data.text,data.status);if(data.status=='success'){el.find('input.second').val('');el.find('td.r'+relation).append($('<div />').append(second,' ',$('<span />').attr({'data-id':data.id,'title':core.tr('erase')}).addClass('delete_js a').text('[x]').click(function(){$this=$(this);core.getJSON('rs_ctrl','delete',[data.id],false,function(data){core.message(data.text,data.status);if(data.status=='success'){$this.parent('div').remove();}});})));}});}});}
el.find('a.delete').click(function(){$this=$(this);core.getJSON('rs_ctrl','delete',[$this.data('id')],false,function(data){core.message(data.text,data.status);if(data.status=='success'){$this.parent('div').remove();}});});});}};