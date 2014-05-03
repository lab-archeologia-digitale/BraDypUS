<?php
/**
 * @author			Julian Bogdani <jbogdani@gmail.com>
 * @copyright		BraDypUS 2007-2012
 * @license			All rights reserved
 * @since			Jul 13, 2012
 * @uses			cfg
 */

class Export
{
	protected $filename, $db, $data, $tb, $sql, $xls;
	
	
	public function __construct(DB $db, $filename, $tb, $where_sql = false)
	{
		
		ini_set('memory_limit','1560M');
		
		$this->filename = $filename;
		$this->tb = $tb;
		$this->db = $db;
		
		$this->data = $db->query('SELECT * FROM `' . $this->tb . '` WHERE ' . ($where_sql ? $where_sql : '1'));
		
		if (!is_array($this->data) OR count($this->data) == 0)
		{
			throw new myException('Table ' . $this->tb . ' is empty!');
		}
	}
	
	public function doExport($format)
	{
		switch($format)
		{
			case 'XLS':
				$this->exportXLS();
				break;
				
			case 'CSV':
				$this->exportCSV();
				break;
				
			case 'SQL':
				$this->exportSQL();
				break;
				
			case 'HTML':
				$this->exportHTML();
				break;
				
			case 'XML':
				$this->exportXML();
				break;
				
			case 'JSON':
				$this->exportJSON();
				break;
				
			default:
				throw new myException('Unknown export format');
				break;
				
		}
	}
	
	private function exportJSON()
	{
		$this->filename .= '.json';
		
		foreach (array_keys($this->data[0]) as $fld_id)
		{
			$labels[] = cfg::fldEl($this->tb, $fld_id, 'label');
		}
		
		$data = array(
				'meta' => array(
						'table' => $this->tb,
						'table_label' => cfg::tbEl($this->tb, 'label'),
						'labels' => $labels,
						'fields' => array_keys($this->data[0]),
						),
				'data' => $this->data
				);
		
		utils::write_formatted_json($this->filename, $data);
	}
	
	
	private function exportXML()
	{
		$this->filename .= '.xml';
		
		$xml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			. "<!--\r"
			. "\tExport from " . strtoupper(PREFIX) . ", table: " . cfg::tbEl($this->tb, 'label') . " (" . $this->tb . ")\n"
			. "\ton " . date('Y-m-d G:i') . "\n"
			. "-->\n"
			. "<root>\n"
				. "\t<table id=\"" . $this->tb . "\" name=\"" . cfg::tbEl($this->tb, 'label') . "\">\n";
				
				foreach ($this->data as $row)
				{
					$xml .= "\t\t<row>\n";
					
					foreach($row as $fld=>$val)
					{
						$xml .= "\t\t\t<fld name=\"" . $fld . "\">" . htmlspecialchars($val) . "</fld>\n";
					}
					$xml .= "\t\t</row>\n";
				}
				
				$xml .= "\t</table>\n"
			. "</root>";
			
			$w = utils::write_in_file($this->filename, $xml);
			
			if (!$w)
			{
				throw new myException('Can not write in file ' . $this->filename);
			}

		
	}
	
	private function exportHTML()
	{
		$this->filename .= '.html';
		
		foreach (array_keys($this->data[0]) as $fld_id)
		{
			$labels[] = cfg::fldEl($this->tb, $fld_id, 'label');
		}
		
		$html = "<!DOCTYPE html >\n"
			. "<html>\n"
			. "<head>\n"
			. "\t<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
			. "\t<title>Export from " . strtoupper(PREFIX) . "</title>\n"
			. "</head>\n"
			. "<body>\n"
			
			. "<h2>Export from " . strtoupper(PREFIX) . ", table: " . cfg::tbEl($this->tb, 'label'). " (" . $this->tb . "), on " . date('Y-m-d G:i') . "</h2>\n"
			
			. "<table width=\"100%\" border=\"1\">\n"
			. "\t<caption>" . cfg::tbEl($this->tb, 'label'). " (" . $this->tb . ")</caption>\n"
			. "\t<thead>\n"
			. "\t<tr>\n"
				. "\t\t<th>"
					. implode("</th>\n\t\t<th>", $labels)
				. "</th>"
			. "\t</tr>\n"
			. "\t</thead>\n"
			. "\t<tbody>\n";
			
			
		foreach ($this->data as $row)
		{
			$html .= "\t<tr>\n"
				. "\t\t<td>" . implode("</td>\n\t\t<td>", $row) . "</td>\n"
			."\t</tr>\n";
		}
		
		$html .= "\t</tbody>\n"
			. "</table>\n"
			. "</body>\n"
			. "</html>";
		
		
		$w = utils::write_in_file($this->filename, $html);
			
		if (!$w)
		{
			throw new myException('Can not write in file ' . $this->filename);
		}
		
	}
	
	
	private function exportXLS()
	{
		$this->filename .= '.xls';
		
		
		foreach (array_keys($this->data[0]) as $fld_id)
		{
			$labels[] = cfg::fldEl($this->tb, $fld_id, 'label');
		}
		
		$this->xls_writeLine($labels);
		
		
		foreach ( $this->data as $row )
		{
			$this->xls_writeLine($row);
		}
		
		$this->xls_close();
	}
	
	/**
	 * 
	 * Increases the row number, and sets column number to 0
	 */
	private function xls_end_row()
	{
		$this->xls['row']++;
		$this->xls['col'] = 0;
	}
	
	/**
	 * 
	 * Write $value in a cell and increases the col number. The value can be a number or a string 
	 * @param mixed $value	int or string. Value to write in cell
	 */
	private function xls_cell($value)
	{
		$value = utf8_decode($value);
		
		if (is_numeric($value))
		{
			$this->xls['string'] .= pack("sssss", 0x203, 14, $this->xls['row'], $this->xls['col'], 0x0);
			
			$value = pack("d", $value);
		}
		else
		{
			$l = strlen($value);
			
			$this->xls['string'] .= pack("ssssss", 0x204, 8 + $l, $this->xls['row'], $this->xls['col'], 0x0, $l);
		}

		$this->xls['string'] .= $value;

		$this->xls['col']++;
	}
	
	/**
	 * 
	 * Packs binary file and writes content to filename. Returns false on error and sets $error with error
	 */
	private function xls_close()
	{
		$content = pack("ssssss", 0x809, 0x8, 0x0, 0x10, 0x0, 0x0);
		
		$content .= $this->xls['string'];
		
		$content .= pack("ss", 0x0A, 0x00);
				
		if(!$hndl = fopen($this->filename, "w+"))
		{
			throw new myException("Can not open " . $this->filename);
		}
		
		if(!fwrite($hndl,$content))
		{
			throw new myException("Can not write in ".$this->filename);
		}
		
		if(!fclose($hndl))
		{
			throw new myException("Can not close ".$this->filename);
		}
	}
	
	/**
	 * 
	 * Writes an array to a line
	 * @param array $arr	array with data to write in line
	 */
	private function xls_writeLine($arr)
	{
		if(!is_array($arr))
		{
			throw new myException("Error : Argument is not valid. Supply an valid Array.");
		}
		
		foreach($arr as $data)
		{
			$this->xls_cell($data);
		}
		
		$this->xls_end_row();
	}
		
	/**
	 * 
	 * Exports sql INSERT statement
	 * @throws myException
	 */
	private function exportSQL()
	{
		$this->filename .= '.sql';
		 
		$sql = '';
		
		$q = $this->data;
		
		for ($x = 0; $x < count($this->data); $x++)
		{
			foreach($this->data[$x] as &$value)
			{
				$value = htmlentities(addslashes($value));
			}
			
			$sql .= "INSERT INTO `{$this->tb}` VALUES " . "('" . implode("','", $this->data[$x]) . "');\n"; 
		}
		
		
		$w = utils::write_in_file($this->filename, $sql);
			
		if (!$w)
		{
			throw new myException('Can not write in file ' . $this->filename);
		}
	}
	
	/**
	 * 
	 * Exports data in csv format
	 * @throws myException
	 */
	private function exportCSV()
	{
		$this->filename .= '.csv';
		
		$delimiter = ',';
		$enclosure = '"';
		
		//open file
		$fh = @fopen($this->filename, 'w+');
		
		if (!$fh)
		{
				throw new myException('Can not open file ' . $this->filename . ' in write mode!');
		}
			
		if (is_array($this->data))
		{
			if (!fputcsv($fh, array_keys($this->data[0]), $delimiter, $enclosure))
			{
				throw new myException('Can not write in file ' . $this->filename . '(1)');
			}
				
			foreach ( $this->data as $line )
			{
				if (!fputcsv($fh, $line, $delimiter, $enclosure))
				{
					throw new myException('Can not write in file ' . $this->filename . ' (2)');
				}
			}
			@fclose($fh);
		}
		else
		{
			throw new myException('Can not export data from ' . $this->tb);
		}
	}
	
}