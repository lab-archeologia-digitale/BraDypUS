<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS, Julian Bogdani <jbogdani@gmail.com>
 * @license			See file LICENSE distributed with this code
 * @since				May 3, 2014
 */

class import_geodata_ctrl extends Controller
{
  public function start()
  {
    $this->render('import_geodata', 'start', [
      'tables' => $this->cfg->get('tables.*.label', 'is_plugin', null)
    ]);
  }
  
  public function load_file()
  {
    $tb = $this->request['tb'];
    
    $this->render('import_geodata', 'load_file', [
      'tb' => $tb
    ]);
  }
  
  public function setFields()
  {
    $tb = $this->get['tb'];
    $file = $this->get['file'];
    
    try {

      $import = new importGeodata();

      $resp = $import->checkGeojson(file_get_contents($file));
      
      $this->render('import_geodata', 'setFields', [
        'tb' => $tb,
        'file' => $file,
        'id_label' => $this->cfg->get("tables.{$tb}.fields." . $this->cfg->get("tables.{$tb}.id_field") . ".label"),
        'resp' => $resp
      ]);
      
    } catch (\Exception $e) {
      $this->log->error($e);
      utils::alert_div('empty_or_wrong_geojson', true);
    }
  }
  
  public function confirm()
  {
    $tb = $this->get['tb'];
    $file = $this->get['file'];
    $id_field = $this->get['id_field'];
    
    $this->render('import_geodata', 'confirm', array(
      'tb' => $tb,
      'tb_label' => $this->cfg->get("tables.{$tb}.label"),
      'file' => $file,
      'id_label' => $this->cfg->get("tables.$tb.fields." . $this->cfg->get("tables.$tb.id_field"). ".label"),
      'id_field' => $id_field
      ));
  }
  
  public function process()
  {
    $tb = $this->get['tb'];
    $file = $this->get['file'];
    $id_field = $this->get['id_field'];
    $delete = $this->get['delete'] === 'yes' ? true :  false;
    
    
    try {
      $import = new importGeodata();
      
      $totalImports = $import->runImport(
        $this->db, 
        $tb, 
        file_get_contents($file), 
        $id_field, 
        $delete,
        $this->cfg->get("tables.$tb.id_field")
      );
      
      echo '<div class="text-success lead"><i class="glyphicon glyphicon-ok"></i> ' 
        . tr::get('geodata_ok_uploaded', [$totalImports] ) . '</div>';
      
    } catch (\Exception $e) {
      $this->log->error($e);
      utils::alert_div('geodata_ok_uploaded', true);
    }
  }
}
?>
