<?php

namespace DB\Alter;


class Mysql implements AlterInterface
{
    private $db;

    public function __construct(\DB $db)
    {
        $this->db = $db;
    }

    public function renameTable(string $old, string $new): bool
    {
        $sql = 'RENAME TABLE `' . $old . '` TO `' . $new . '`';
        return  $this->db->execInTransaction($sql);
    }

    public function renameFld(string $tb, string $old, string $new, $fld_type = false): bool
    {
        $sql = "ALTER TABLE `{$tb}` CHANGE `{$old}` `$new` {$fld_type}";
        return $this->db->execInTransaction($sql);
    }

    public function addFld(string $tb, string $fld_name, string $fld_type): bool
    {
        $sql = "ALTER TABLE `{$tb}` ADD `{$fld_name}` {$fld_type}";
        return $this->db->execInTransaction($sql);
    }

    public function dropFld(string $tb, string $fld_name): bool
    {
        $sql = "ALTER TABLE `{$tb}` DROP COLUMN `{$fld_name}`";
        return  $this->db->execInTransaction($sql);
    }
}