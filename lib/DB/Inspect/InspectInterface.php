<?php

/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 */

namespace DB\Inspect;

/**
 * Interface to interact with database structure
 */
interface InspectInterface
{
  /**
   * Checks if table exists
   */
  public function tableExists(string $tb): bool;

  /**
   * Gets list of table columns 
   *
   * @param string $tb    Table name
   * @return array array of arrays with column data
   *                  example: [ [ 'fld' => "fld_name", 'type' => 'column_type' ], [ ...etc... ] ]
   */
  public function tableColumns(string $tb): array;

  /**
   * Returns full list of tables contained in database
   *
   * @return array
   */
  public function getAllTables(): array;
}
