<?php
/**
 * @copyright 2007-2021 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 */

declare(strict_types=1);

namespace SQL\ShortSql;

use SQL\SqlException;
use SQL\ShortSql\Order;
use SQL\ShortSql\Limit;
use SQL\ShortSql\Group;
use SQL\ShortSql\Where;
use SQL\ShortSql\SubQuery;
use SQL\ShortSql\Join;

use SQL\QueryObject;
use Config\Config;


class ParseShortSql
{
    private $parts;

    private $added_tb_aliases = [];
    private $added_fld_aliases = [];

    private $prefix;
    private $cfg;
    private $qo;

    public function __construct(string $prefix = null, Config $cfg = null)
    {
        $this->prefix = $prefix;
        $this->cfg = $cfg;
        $this->qo = new QueryObject(false, $this->cfg);
        $this->parts = [
            'tb'        => null, // tb, [tb, alias]
            'fields'    => null, // fld1,fld2, [[tb, fld1, fld1_alias], [ ... ]]
            'join'      => null, // [ [tb, on, values], [...] ]
            'where'     => null, // [ [null, fld, op, val ], [conn, fld, op, val] ]
            'group'     => null,
            'order'     => null,
            'limit'     => null,
            'values'    => null,
        ];
        $this->symbols = [
            '@' => 'tb'    ,
            '[' => 'fields',
            ']' => 'join'  ,
            '?' => 'where' ,
            '*' => 'group' ,
            '>' => 'order' ,
            '-' => 'limit'
            // RESERVED: ^ value is fieldame, not string
            // RESERVED: < introduces subquery
        ];
    }

    public function parseAll(string $str): self
    {
        // Explode string
        $parts = explode('~', $str);

        // And set parts
        foreach ($parts as $part) {
            foreach ($this->symbols as $symbol => $key) {
                if ($part[0] === $symbol) {
                    if($key === 'join') {
                        $this->parts[$key][] = substr($part, 1);
                    } else {
                        $this->parts[$key] = substr($part, 1);
                    }
                }
            }
        }

        $this->parseParts();
        return $this;
    }
    
    public function getQueryObject()
    {
        return $this->qo;
    }

    /**
     * Alias of QueryObject::getSql
     * Returns array containing as
     *  first element the SQL statement, if $onlyWhere is truw, only the WHERE parte will be returned
     *  second element, an array with values
     * @param bool $onlyWhere
     * @return array
     */
    public function getSql(bool $onlyWhere = false) : array
    {
        return $this->qo->getSql($onlyWhere);
    }


    /**
     * Sets queryobject
     *
     * @return void
     */
    private function parseParts()
    {
        $parsedTb = Table::parse($this->prefix, $this->parts['tb']);
        $tb = $parsedTb['tb'];
        $tb_alias = $parsedTb['alias'];
        unset($parsedTb);
        
        // Table
        $this->qo->setTb($tb, $tb_alias);

        // Group & Fields
        $group = Group::parse($this->prefix, $this->parts['group'], $tb);
        if (!empty($group)){
            foreach ($group as $g) {
                // If grouping is active, fields are set from grouping
                $this->qo->setField($g);
                $this->qo->setGroupFld($g);
            }
        } else {
            // Fields are set only if grouping is off
            $fields = $this->parseFldList($this->parts['fields'], $tb);
            foreach ($fields as $f) {
                $this->qo->setField($f[1], $f[2], $f[0], $f[3]);
            }
        }
        

        // Explixit joins
        $joins = Join::parse(
            $this->prefix, 
            ($this->parts['join'] ?? []),
            $this->cfg,
            new self($this->prefix, $this->cfg),
            $this->added_fld_aliases
        );

        foreach ($joins as $j) {
            $this->qo->setJoin($j['tb'], $j['alias'], $j['on']);
        }
        
        // Where & Auto-Joins resolved by Where fields
        if ($this->parts['where'] && $this->parts['where'] !== '1') {

            $parsedWhere = Where::parse(
                $this->cfg, 
                $this->parts['where'], 
                $tb, 
                false, 
                $this->added_fld_aliases, 
                new self($this->prefix, $this->cfg),
                $this->prefix
            );
            $where = $parsedWhere['sql_parts'];
            $values = $parsedWhere['sql_values'];
            unset($parsedWhere);

            foreach ($where as $w) {
                /*
                connector
                opened_bracket
                fld
                operator
                binded
                closed_bracket
                */
                $this->qo->setWherePart(
                    $w['connector'], 
                    $w['opened_bracket'], 
                    $w['fld'], 
                    $w['operator'], 
                    $w['binded'], 
                    $w['closed_bracket']
                );
            }
            $this->qo->setWhereValues($values);
        }
        
        // Order
        $order = Order::parse($this->prefix, $this->parts['order'], $tb);
        foreach ($order as $o) {
            $this->qo->setOrderFld($o['fld'], $o['dir']);
        }
        
        // Limit
        $parsedLimit = Limit::parse($this->parts['limit']);
        if (isset($parsedLimit['rows']) && isset($parsedLimit['offset'])) {
            $this->qo->setLimit((int)$parsedLimit['rows'], (int)$parsedLimit['offset']);
        }

    }


    /**
     * Parses string of fields, eg.: null, '*', 'tb.*', 'fld1,fldn', 'tb.fld1,fldn
     * And returns array of fields, providing for each : table name, field name, field alias, function
     *
     * @param array $fields_arr
     * @param string $tb
     * @return array
     */
    private function parseFldList ( string $fields = null, string $tb ) : array
    {
        if (!$fields || $fields === '*' ){
            return [
                [ $tb, '*', null, null ]
            ];
        }

        $formatted_flds = [];
        $fields_arr = explode(',', $fields);

        foreach ($fields_arr as $f) {

            $parsedFld = Field::parse($this->prefix, $f, $tb, new self($this->prefix, $this->cfg));
            $tbf        = $parsedFld['tb'];
            $fld        = $parsedFld['fld'];
            $alias      = $parsedFld['alias'];
            $function   = $parsedFld['fn'];
            unset($parsedFld);
            array_push($formatted_flds, [
                $tbf, $fld, $alias, $function
            ]);
        }
        return $formatted_flds;
    }

}