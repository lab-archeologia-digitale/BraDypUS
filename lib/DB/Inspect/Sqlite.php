<?php

/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 */

namespace DB\Inspect;

use DB\DBInterface;

/**
 * SQLite implementation of Inspect Interface
 */
class Sqlite implements InspectInterface
{
  private $db;

  public function __construct(DBInterface $db)
  {
    $this->db = $db;
  }

  public function tableExists(string $tb): bool
  {
    $res = $this->db->query(
      "SELECT count(name) as tot FROM sqlite_master WHERE type='table' AND name = ?",
      [$tb]
    );

    return $res && !empty($res) && $res[0]['tot'] > 0;
  }

  public function tableColumns(string $tb): array
  {
    $ret = [];

    $res = $this->db->query("PRAGMA table_info('{$tb}')");
    if (!$res || empty($res)) {
      throw new \Exception("Error on getting column list for table {$tb}");
    }

    foreach ($res as $row) {
      $ret[] = [
        'fld' => $row['name'],
        'type' => $row['type']
      ];
    }

    return $ret;
  }

  public function getAllTables(): array
  {
    $res = $this->db->query("SELECT name FROM sqlite_master WHERE type ='table'");

    $ret = [];
    foreach ($res as $row) {
      array_push($ret, $row['name']);
    }
    return $ret;
  }
}
