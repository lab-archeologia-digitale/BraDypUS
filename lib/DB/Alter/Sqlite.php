<?php
/**
 * @copyright 2007-2022 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 */

namespace DB\Alter;

use DB\DBInterface;

class Sqlite implements AlterInterface
{
    private $sqlite_version;
    private $db;

    public function __construct(DBInterface $db)
    {
        $sqlite_version_arr = \SQLite3::version();
        $this->sqlite_version = $sqlite_version_arr['versionString'];
        $this->db = $db;
    }

    public function renameTable(string $old, string $new): bool
    {   
        $sql = 'ALTER TABLE ' . $old . ' RENAME TO ' . $new;
        return $this->db->execInTransaction($sql);        
    }

    public function renameFld(string $tb, string $old, string $new, $fld_type = false): bool
    {
        if( \version_compare($this->sqlite_version, '3.25.0')  === -1 ) {
            // TODO: implement the old way
            // https://www.sqlitetutorial.net/sqlite-rename-column/#:~:text=ALTER%20TABLE%20table_name%20RENAME%20COLUMN,name%20after%20the%20TO%20keyword.
            throw new \Exception("Your sqlite version ({$this->sqlite_version}) does not support field rename");
            

        } else {
            $sql = "ALTER TABLE {$tb} RENAME COLUMN {$old} TO {$new}";
            return $this->db->execInTransaction($sql);
        }
    }

    public function addFld(string $tb, string $fld_name, string $fld_type): bool
    {
        $sql = "ALTER TABLE {$tb} ADD COLUMN {$fld_name} {$fld_type}";
        return $this->db->execInTransaction($sql);
    }

    public function dropFld(string $tb, string $fld_name): bool
    {
        // https://www.sqlitetutorial.net/sqlite-rename-column/
        
        // Get create table sql text
        $res = $this->db->query('SELECT * FROM sqlite_master WHERE name = ?', [ $tb ] );
        if (!$res || !\is_array($res) || !$res[0]['sql'] ) {
            throw new \Exception("Cannot get create SQL for $tb");
        }
        
        $orig_sql = $res[0]['sql'];

        // get list of fields
        $res2 = $this->db->query('PRAGMA table_info(' . $tb . ')');
        if (!$res2 || !\is_array($res2) ) {
            throw new \Exception("Cannot get fieldsfor $tb");
        }
        foreach ($res2 as $row) {
            if ($row['name'] !== $fld_name) {
                $fields[] = $row['name'];
            }
        }

        $fields = '"' . implode('", "', $fields). '"';
        
        $tmp_tb = uniqid($tb);

        $sql['create_tmp'] = preg_replace(
            [
                '/CREATE\s+TABLE\s+"?' . $tb. '"?/im',
                '/"?\b' . $fld_name . '\b"?[^\),]+,?/im',
                '/,\s*\)/im',
            ],
            [
                'CREATE TABLE "' . $tmp_tb. '"',
                '',
                ')'
            ],
            $orig_sql
        );
        $sql['insert'] = "INSERT INTO $tmp_tb ( $fields ) SELECT $fields FROM $tb";
        $sql['drop'] = "DROP TABLE $tb";
        $sql['rename'] = "ALTER TABLE $tmp_tb RENAME TO $tb";

        try {
            $this->db->beginTransaction();

            foreach ($sql as $k => $s) {
                if ( $this->db->exec($s) === false ){
                    throw new \Exception("Error in query $s");
                }
            }

			$this->db->commit();
		} catch (\Throwable $th) {
            $this->db->rollBack();
            return false;
        }
        
        return true;
    }

    public function createMinimalTable( string $tb, bool $is_plugin = false ): bool
    {
        $sql = "CREATE TABLE IF NOT EXISTS {$tb} (id INTEGER PRIMARY KEY AUTOINCREMENT, ";
        
        if ($is_plugin) {
            $sql .= "table_link TEXT NOT NULL, id_link INTEGER NOT NULL";
        } else {
            $sql .= "creator INTEGER NOT NULL";
        }

        $sql .= ")";

        return $this->db->execInTransaction($sql);
    }

    public function dropTable( string $tb): bool
    {
        $sql = "DROP TABLE IF EXISTS {$tb}";
        return $this->db->execInTransaction($sql);
    }
}