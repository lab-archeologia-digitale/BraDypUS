<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS 2007-2012
 * @license			All rights reserved
 * @since			Apr 21, 2012
 * @uses			cfg
 * 
 * $data['core'] = array of core data [field_id=>value]
 * $data['plugins'] = array of arrays od plugin data [plugin_name=>[fld_id=>value]]
 * $data['corelinks_details'] = array of arrays links id and id_field: [table_name => [ [id=int, id_field=string], [...]], [...] ]
 * $data['corelinks'] = array with sum-up info about corelinks [table_name => [tot:int, query:string, values:array], [...]]
 * $data['userlinks'] = 
 * $data['files'] = 
 * $data['geodata']
 */

class Record implements iRecord
{
	protected $tb, $id, $db, $data, $loaded, $changed, $idField, $tmp_dir;
	
	public function __construct($tb, $id = false, DB $db)
	{
		$this->tb = $tb;
		
		if ($id == 'last_added')
		{
			$t = $db->query('SELECT `id` FROM `' . $tb . '` WHERE 1 ORDER BY `id` DESC LIMIT 0,1', false, 'read');
			$id = $t[0]['id'];
		}
		$this->id = $id;
		$this->db = $db;
		$this->tmp_dir = PROJ_TMP_DIR;
	}
	
	public function getId()
	{
		return $this->id;
	}
	
	public function setTmpFolder($dir)
	{
		$this->tmp_dir = $dir;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getTb()
	 */
	public function getTb()
	{
		return $this->tb;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getDB()
	 */
	public function getDB()
	{
		return $this->db;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::persist()
	 */
	public function persist()
	{
		if (is_array($this->changed['core']) AND count($this->changed['core']) > 0)
		{
		// CORE.update
			if ($this->id AND !empty($this->id))
			{
				foreach ($this->data['core'] as $key => $val)
				{
					if ($key !== 'id' AND in_array($key, $this->changed['core']))
					{
						$tmp[] = "'" . $key . "' = :" . $key;
					
						$values[':' . $key] = $val;
					}
				}
				
				$query = 'UPDATE `' . $this->tb . '` SET ' . implode(', ', $tmp) . ' WHERE `id` = ' . $this->id;
				
				$flag['core'] = $this->db->query($query, $values, 'boolean');
				
				// moves updaded file
				if ($this->tb == PREFIX . '__files')
				{
					if (file_exists($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext']))
					{
						@copy($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'], PROJ_FILES_DIR . $this->data['core']['id'] . '.' . $this->data['core']['ext']);
					}
				}
				
			}
			else
			// CORE.insert
			{
				
				foreach ($this->data['core'] as $key => $val)
				{
					$values[':' . $key] = $val; 
				}
				$values[':creator'] = $_SESSION['user']['id'];
				$query = 'INSERT INTO `' . $this->tb . "` (`creator`, '" . implode("', '", array_keys($this->data['core'])) . "') VALUES (:creator, :" . implode(', :', array_keys($this->data['core'])). ")";
				
				$last_id = $this->db->query($query, $values, 'id');
				
				// moves updaded file
				if ($this->tb == PREFIX . '__files')
				{
					if (file_exists($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext']))
					{
						@copy($this->tmp_dir . $this->data['core']['filename'] . '.' . $this->data['core']['ext'], PROJ_FILES_DIR . $last_id . '.' . $this->data['core']['ext']);
					}
				}
				
				
				$flag['core'] = $last_id;
				
				$this->data['core']['id'] = $last_id;
				
				$this->id = $last_id;
			}
		}
		
		// RS
		if ($this->changed['rs'])
		{
			$q_arr = array();
			
			foreach ($this->data['rs'] as $rel)
			{
				if (!$rel['id'])
				{
					array_push($q_arr, array(
							'q' => 'INSERT INTO `' . PREFIX . '__rs` (`tb`, `first`, `second`, `relation`) VALUES (:tb, :first, :second, :relation)',
							'values'=>array(':tb'=>$tb, ':first'=>$rel['first'], ':second'=>$re['second'], ':relation'=>$rel['relation'])
							));
				}
			}
			$flag['rs'] = $this->query2db($q_arr);
		}
			
		
		//PLUGINS
		if (is_array($this->data['plugins']))
		{
			$query = array();
			
			foreach ($this->data['plugins'] as $plg_name => $plg_all_rows)
			{
				foreach ($plg_all_rows as $plg_row)
				{
					$tmp = array();
					$values = array();
					
					$has_value_flag = false;
					
					foreach ($plg_row as $key => $val)
					{
						if ($key != 'table_link' AND $key != 'id_link' AND !empty($val))
						{
							$has_value_flag = true;
						}
						if ($key != 'id')
						{
							$tmp[] = "'" . $key . "' = :" . $key;
			
							$values[':' . $key] = $val;
						}
					}
					// plg.row.DELETE
					if($plg_row['id_delete'] AND !empty($plg_row['id_delete']))
					{
						array_push($query, array(
								'q' => 'DELETE FROM `' . $plg_name . '` WHERE `id` = ' . $plg_row['id_delete'],
								'values' => false
						));
					}
					// plg.row.UPDATE
					else if($plg_row['id'] AND !empty($plg_row['id']))
					{
						array_push($query, array(
								'q' => 'UPDATE `' . $plg_name . '` SET ' . implode(', ', $tmp) . ' WHERE `id` = ' . $plg_row['id'],
								'values' => $values
						));
					}
					// plg.row.INSERT
					else
					{
						if ($has_value_flag)
						{
							// if context is add new must re-set the value of core_id manually!
							if (empty($plg_row['id_link']) AND $this->data['core']['id'])
							{
								$plg_row['id_link'] = $this->data['core']['id'];
								$values[':id_link'] = $this->data['core']['id'];
							}
							array_push($query, array(
									'q' => 'INSERT INTO `' . $plg_name . "` ('" . implode("', '", array_keys($plg_row)) . "') VALUES (:" . implode(', :', array_keys($plg_row)). ")",
									'values' => $values
							));
						}
					}
				}
			}
			if (is_array($query))
			{
				$this->db->beginTransaction();
				
				foreach ($query as $q_arr)
				{
					$ret = $this->db->query($q_arr['q'], $q_arr['values'], 'boolean');
					
					$flag['plugins'][($ret ? 'success' : 'error')][] = true;
				}
				
				$this->db->commit();
			}
		}
		
		return $flag;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::delete()
	 */
	public function delete()
	{
		$id = $this->id ? $this->id : $this->data['core']['id'];
		
		if (!$id)
		{
			return false;
		}
		
		// CORE
		$q[] = 'DELETE FROM `' . $this->tb . '` WHERE `id` = ' . $id;
		
		// PLUGINS
		$plugins = cfg::tbEl($this->tb, 'plugin');
		
		if (is_array($plugins))
		{
			foreach ($plugins as $plg)
			{
				$q[] = 'DELETE FROM `' . $plg . '` WHERE `table_link` = \'' . $this->tb. '\' AND `id_link` = ' . $id;
			}
		}
		
		// USERLINKS
		$q[] = 'DELETE FROM `' . PREFIX . '__userlinks` WHERE '
			." ( `tb_one` = '" . $this->tb . "' AND `id_one` = " . $id . ")"
			. ' OR '
			. "( `tb_two` = '" . $this->tb . "' AND `id_two` = " . $id . ")";
		  
		//GEODATA
		$q[] = "DELETE FROM `" . PREFIX . "__geodata` WHERE  `table_link` = '" . $this->tb . "' AND `id_link` = " . $id;
		
		//DELETE FILE (deleted only if all db erases are OK)
		if ($this->tb == PREFIX . '__files')
		{
			$file_to_delete = PROJ_FILES_DIR . $id . '.' . $this->getCore('ext');
		}
		
		// RS
		$rs_fld = cfg::tbEl($this->tb, 'rs');
		if ($rs_fld)
		{
			if (!$this->loaded['core'])
			{
				$this->getCore();
			}
			
			$q[] = 'DELETE FROM `' . PREFIX . '__rs` WHERE ' . "`tb` = '" . $this->tb . "' AND ( "
						. " `first` =  '" . $this->data['core'][$rs_fld] . "' "
						. ' OR '
						. " `second` =  '" . $this->data['core'][$rs_fld] . "' "
					. ' )';
						
		}
		
		
		$this->db->beginTransaction();
		
		try
		{
			foreach ($q as $query)
			{
				$this->db->query($query);
			}
			$this->db->commit();
			
			if ($file_to_delete and file_exists($file_to_delete))
			{
				@unlink($file_to_delete);
			}
			
			return true;
		}
		catch (myException $e)
		{
			$this->db->rollBack();
			throw new myException($e);
		}
		
	}
	
	/**
	 * Sets idField value
	 * @param string $idField
	 */
	public function setIdField($idField)
	{
		$this->idField = $idField;
	}
	
	/**
	 * 
	 * Loads core data
	 */
	private function loadCore()
	{
		if ($this->id)
		{
			$query = "SELECT * FROM `" . $this->tb . "` WHERE `id` = :id";
		
			$values = array(':id' => $this->id);
		}
		else if ($this->idField)
		{
			$query = "SELECT * FROM `" . $this->tb . "` WHERE `" . cfg::tbEl($this->tb, 'id_field'). "` = :id_field LIMIT 0, 1";
			
			$values = array(':id_field' => $this->idField);
		}
		
		$res = $this->db->query($query, $values, 'read');
		
		$this->loaded['core'] = true;
		
		if (is_array($res[0]))
		{
			$this->data['core'] = $res['0'];
		}
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::setCore()
	 */
	public function setCore($core_data)
	{
		if (!$this->loaded['core'])
		{
			$this->loadCore();
		}
		
		if (!is_array($this->data['core']))
		{
			// it means that no id was found => it's a new record
			$this->data['core'] = array();
		}
		
		$this->data['core'] = array_merge($this->data['core'], $core_data[$this->tb]);
		
		$this->changed['core'] = array_keys($core_data[$this->tb]);
		
		$this->loaded['core'] = true;
	}
	
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getCore()
	 */
	public function getCore($fieldname = false)
	{
		if (!$this->loaded['core'])
		{
			$this->loadCore();
		}
		if ($fieldname)
		{
			return $this->data['core'][$fieldname];
		}
		else
		{
			return $this->data['core'];
		}
	}
	
	/**
	 * 
	 * Loads specifica plugin data
	 * @param string $plugin plugin name
	 * @param int	$core_id id of main table
	 */
	private function loadPlugin($plugin, $core_id = false)
	{
		$core_id = $core_id ? $core_id : $this->data['core']['id'];
		 
		if (!$core_id)
		{
			$this->loaded['plugins'][$plugin] = true;
			return;
		}
		$query = "SELECT * FROM `" . $plugin . "` WHERE `table_link` = :table_link AND `id_link` = :id_link";
		
		$res = $this->db->query($query, array(':table_link'=>$this->tb, ':id_link' => $core_id));
		
		$this->loaded['plugins'][$plugin] = true;
		
		if (count($res) > 0)
		{
			$this->data['plugins'][$plugin] = $res;
		}
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::setPlugin()
	 */
	public function setPlugin($plugin, $plugin_data)
	{
		if ($this->id)
		{
			$this->loadAllPlugins($this->id);
		}
		
		foreach ($plugin_data as $row_id => &$row)
		{
			$row['table_link'] = $this->tb;
			
			$row['id_link'] = $this->id;
		}
		if (!is_array($this->data['plugins'][$plugin]))
		{
			$this->data['plugins'][$plugin] = array();
		}
		$this->data['plugins'][$plugin] = array_merge($this->data['plugins'][$plugin], $plugin_data);
		
		$this->changed['plugins'][plugin] = true;
		
		$this->loaded['plugins'][$plugin] = true;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getPlugin()
	 */
	public function getPlugin($plugin, $index = false, $fieldname = false)
	{
		$this->getCore();
		
		if (!$this->loaded['plugins'][$plugin])
		{
			$this->loadPlugin($plugin);
		}
		if ($index !== false AND $fieldname)
		{
			return $this->data['plugins'][$plugin][$index][$fieldname];
		}
		else
		{
			return $this->data['plugins'][$plugin];
		}
	}
	
	/**
	 * 
	 * Loads all plugin data
	 * @param int	$core_id id of main table
	 */
	private function loadAllPlugins($core_id)
	{
		$plugins = cfg::tbEl($this->tb, 'plugin');
		
		if (!$plugins)
		{
			$this->loaded['allPlugins'] = true;
			return;
		}
		
		foreach ($plugins as $plugin)
		{
			$this->loadPlugin($plugin, $core_id);
		}
		$this->loaded['allPlugins'] = true;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getAllPlugins()
	 */
	public function getAllPlugins($load = false)
	{
		$this->getCore();
		
		if ($load AND !$this->loaded['allPlugins'])
		{
			$this->loadAllPlugins();
		}
		
		return $this->data['plugins'];
	}
	
	/**
	 * Loads link data
	 * @param boolean $detail
	 */
	private function loadCoreLinks($detail = false)
	{
		$links = cfg::tbEl($this->tb, 'link');
		
		if (!$links)
		{
			$this->loaded['corelinks'] = true;
			return;
		}
		
		if (!$this->loaded['core'])
		{
			$this->loadCore();
		}
		
		foreach ($links as $link)
		{
			/*
			 * $link = array('other_tb' => string, 'fld'=>array( 0 => array('other': string, 'my'=>string))
			 * if !is_array(fld)	:	error
			 * if !is_array(fld[0])	:	error
			 */
			
			if ( !is_array($link['fld']) || !is_array($link['fld'][0]) )
			{
				error_log('Configuration problem. Tables > `' . $this->tb . '` > links > `' . $link['other_tb'] . '` NOT well formatted and will be ignored');
				continue;
			}
			
			$tmp = array();
			
			foreach ($link['fld'] as $fld_arr)
			{
				$tmp[] = ' `' . $fld_arr['other'] . "` ='" . $this->data['core'][$fld_arr['my']] . "' ";
			}
				
			$id_fld = cfg::tbEl($link['other_tb'], 'id_field');
			
			$query = "SELECT `id` " . ( ($id_fld == 'id') ? '' : ', `' . $id_fld . '`') . " FROM `" . $link['other_tb'] . "` WHERE " . implode(' AND ', $tmp);
			
			$res = $this->db->query($query, false, 'read');
			
			if ($res)
			{
				if (!$detail)
				{
					$this->data['corelinks'][$link['other_tb']]['tot'] = count($res);
					$this->data['corelinks'][$link['other_tb']]['query'] = implode(' AND ', $tmp);
				}
				else
				{
					foreach ($res as $row)
					{
						$this->data['corelinks_details'][$link['other_tb']][] = $row;
					}
				}
			}
			
				
		}
		$detail ?  ($this->loaded['corelinks_details'] = true) : ($this->loaded['corelinks'] = true);
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getCoreLinks()
	 */
	public function getCoreLinks($details = false)
	{
		if ($details)
		{
			if (!$this->loaded['corelink_details'])
			{
				$this->loadCoreLinks(true);
			}
			return $this->data['corelinks_details'];
		}
		else
		{
			if (!$this->loaded['corelink'])
			{
				$this->loadCoreLinks();
			}
			return $this->data['corelinks'];
		}
	}
	
	/**
	 * 
	 * Load full data for linked file(s) 
	 */
	private function loadFullFiles()
	{
		
		if ($this->tb == PREFIX . '__files')
		{
			if (!$this->loaded['core'])
			{
				$this->loadCore();
			}
			$this->data['fullfiles'][0] = $this->data['core']; 
		}
		else
		{
			if ($this->id)
			{
				$query = 'SELECT `' . PREFIX . '__files`.*, `' . PREFIX . '__userlinks`.`id` as linkid'
						. ' FROM `' . PREFIX . '__files` JOIN `' . PREFIX . '__userlinks`'
						. ' ON ( '
							. "(`" . PREFIX . "__userlinks`.`tb_one` = '" . PREFIX . "__files' AND  `" . PREFIX . "__userlinks`.`id_one` = `" . PREFIX . "__files`.`id`)"
							. ' OR '
							. "(`" . PREFIX . "__userlinks`.`tb_two` = '" . PREFIX . "__files' AND  `" . PREFIX . "__userlinks`.`id_two` = `" . PREFIX . "__files`.`id`)"
						. ')'
						. ' WHERE '
							. "(`" . PREFIX . "__userlinks`.`tb_one` = '" . $this->tb . "' AND  `" . PREFIX . "__userlinks`.`id_one` = " . $this->id . ")"
							. ' OR '
							. "(`" . PREFIX . "__userlinks`.`tb_two` = '" . $this->tb . "' AND  `" . PREFIX . "__userlinks`.`id_two` = " . $this->id . ")"
						. ' ORDER BY `' . PREFIX . '__userlinks`.`sort`, `' . PREFIX . '__userlinks`.`id`';
				$this->data['fullfiles'] = $this->db->query($query, false, 'read');
			}
		}
			
		$this->loaded['fullfiles'] = true;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getFullFiles()
	 */
	public function getFullFiles()
	{
		if (!$this->loaded['fullfiles'])
		{
			$this->loadFullFiles();
		}
		return $this->data['fullfiles'];
	}
	
	private function loadRS()
	{
		$query = 'SELECT * FROM `' . PREFIX . '__rs` WHERE `tb`=:table AND (`first`=:id OR `second` = :id)';
		
		$this->data['rs'] = $this->db->query($query, array(':table'=>$this->tb, ':id'=>$this->id), 'read');
		
		$this->loaded['rs'] = true;
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::getRS()
	 */
	public function getRS()
	{
		if (!$this->loaded['rs'])
		{
			$this->loadRS();
		}
		return $this->data['rs'];
	}
	
	/**
	 * (non-PHPdoc)
	 * @see iRecord::setRS()
	 */
	public function addRS($first, $relation, $second)
	{
		$first = trim($first);
		$second = trim($second);
		$res = $this->db->query(
				'SELECT * FROM `' . PREFIX . '__rs` WHERE `tb` = :tb AND ( (`first` = :first AND `second` =:second) OR (`first`=:second AND `second` = :first) )',
				array(':tb'=>$this->tb, ':first'=>$first, ':second'=>$second),
				'read'
		);
		
		if (count($res) > 0)
		{
			return false;
		}
		else
		{
			return $this->db->query(
					'INSERT INTO `' . PREFIX . '__rs` (`tb`, `first`, `second`, `relation`) VALUES (:tb, :first, :second, :relation)',
					array(':tb'=>$this->tb, ':first'=>$first, ':second'=>$second, ':relation'=>$relation),
					'id'
			);
		}
	}
	
	
	public function deleteRS($id)
	{
		return $this->db->query('DELETE FROM `' . PREFIX . '__rs` WHERE `id`=' . $id, false, 'boolean');
	}
	
	/**
	 * Executes all queries of a query array (q=> query text, values=>array with values to bind)
	 * and returns array with result count flag (array(success => array(), error=>array())
	 * @param unknown_type $query_arr
	 */
	private function query2db($q_arr)
	{
		$flag = array();
		
		if (is_array($q_arr))
		{
			$this->db->beginTransaction();
		
			foreach ($q_arr as $q)
			{
				$ret = $this->db->query($q['q'], $q['values'], 'boolean');
					
				$flag[($ret ? 'success' : 'error')][] = true;
			}
		
			$this->db->commit();
		}
		return $flag;
	}
	
	
	public function getUserLinks()
	{
		if (!$this->loaded['userlinks'])
		{
			$this->loadUserLinks();
		}
		
		return $this->data['userlinks'];
	}
	
	private function loadUserLinks()
	{
		$sql = 'SELECT * FROM `' . PREFIX . '__userlinks` WHERE (`tb_one` = :thistb AND `id_one` = :thisid) OR (`tb_two` = :thistb AND `id_two` = :thisid)';
		
		$userlinks = $this->db->query($sql, array(':thistb' =>$this->tb, ':thisid' =>$this->id), 'read');
		
		if ($userlinks)
		{
			foreach ($userlinks as $userlink)
			{
				if ($this->tb == $userlink['tb_one'] AND $this->id == $userlink['id_one'])
				{
					$ul[] = array('id'=>$userlink['id'], 'tb'=>$userlink['tb_two'], 'ref_id'=>$userlink['id_two']); 
				}
				else if ($this->tb == $userlink['tb_two'] AND $this->id == $userlink['id_two'])
				{
					$ul[] = array('id'=>$userlink['id'], 'tb'=>$userlink['tb_one'], 'ref_id'=>$userlink['id_one']); 
				}
			}
		}
		$this->data['userlinks'] = $ul;
		
		$this->loaded['userlinks'] = true;
	}
	
	public function deleteUserLink($id)
	{
		return $this->db->query('DELETE FROM `' . PREFIX . '__userlinks` WHERE `id`=' . $id, false, 'boolean');
	}
	
	public function addUserLink($tb, $id)
	{
		return $this->db->query(
				'INSERT INTO `' . PREFIX . '__userlinks` (`tb_one`, `id_one`, `tb_two`, `id_two`) VALUES (:thistb, :thisid, :tb, :id)',
				array(':thistb'=>$this->tb, ':thisid'=>$this->id, ':tb' => $tb, ':id' => $id),
				'boolean'
				);
	}
	
	public function getGeodata()
	{
		if (!$this->loaded['geodata'])
		{
			$this->loadGeodata();
		}
		
		return $this->data['geodata'];
	}
	
	private function loadGeodata()
	{
		$this->data['geodata'] = $this->db->query(
				'SELECT * FROM `' . PREFIX . '__geodata` WHERE `table_link` = :tb AND `id_link`= :id',
				array(':tb'=>$this->tb, ':id'=>$this->id),
				'read');
		$this->loaded['geodata'] = true;
	}
	
	public function addGeodata($geometry)
	{
		return  $this->db->query(
				'INSERT INTO `' . PREFIX . '__geodata` (`table_link`, `id_link`, `geometry`) VALUES(:table_link, :id_link, :geometry)',
				array(':table_link'=>$this->tb, ':id_link'=>$this->id, ':geometry' => $geometry),
				'id'
		);
	}
	
	public function deleteGeodata($id)
	{
		return $this->db->query('DELETE FROM `' . PREFIX . '__geodata` WHERE `id`= ' . $id, false, 'boolean');
	}

}