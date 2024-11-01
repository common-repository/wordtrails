<?php
/**
 * This file contains WordPress' update functions
 */
require_once(ABSPATH . 'wp-admin/includes/upgrade.php');

WordTrailsUtilities::setTables();
$wt_tables = WordTrailsGlobal::$tables;

$sql = "CREATE TABLE $wt_tables->external (
  ExternalID bigint(20) NOT NULL auto_increment,
  URL blob NOT NULL,
  Site blob,
  XML blob,
  PRIMARY KEY  (ExternalID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->node (
  NodeID bigint(20) NOT NULL auto_increment,
  Hash varchar(50) NOT NULL,
  Type varchar(50) NOT NULL default 'node',
  Slug varchar(100) default NULL,
  isInternal tinyint(1) NOT NULL default '1',
  ReferenceID bigint(20) default NULL,
  Name varchar(100) default NULL,
  ShortDesc varchar(150) default NULL,
  LongDesc varchar(500) default NULL,
  X float default NULL,
  Y float default NULL,
  Created timestamp NULL default NULL,
  Modified timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (NodeID),
  UNIQUE KEY Hash (Hash)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$checkDefSQL = "SHOW COLUMNS FROM $wt_tables->node_rel LIKE 'Default'";
$res = $wpdb->query($checkDefSQL);
if ((int)$res) {
    $alterDefaultSQL = "ALTER TABLE $wt_tables->node_rel CHANGE `Default` `Def` TINYINT( 1 ) NOT NULL DEFAULT '0'";
    $wpdb->query($alterDefaultSQL);
}

$sql = "CREATE TABLE $wt_tables->node_rel (
  NodeRelationshipID bigint(20) NOT NULL auto_increment,
  ParentID bigint(20) NOT NULL,
  ChildID bigint(20) NOT NULL,
  Comment varchar(255) default NULL,
  Def tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (NodeRelationshipID),
  UNIQUE KEY Relationship (ParentID,ChildID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->node_tag (
  NodeTagID bigint(20) NOT NULL auto_increment,
  NodeID bigint(20) NOT NULL,
  TagID bigint(20) NOT NULL,
  PRIMARY KEY  (NodeTagID),
  UNIQUE KEY NodeTag (NodeID,TagID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->tag (
  TagID bigint(20) NOT NULL auto_increment,
  Tag varchar(50) NOT NULL,
  PRIMARY KEY  (TagID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->push (
  PushID bigint(20) NOT NULL auto_increment,
  TrailID bigint(20) NOT NULL,
  Stamp timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (PushID),
  UNIQUE  (TrailID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->usage (
  UsageID BIGINT NOT NULL auto_increment,
  Stamp timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  Type VARCHAR( 50 ) NULL DEFAULT 'WEV',
  TrailHash VARCHAR( 50 ) NOT NULL ,
  NodeHash VARCHAR( 50 ) NULL ,
  SessionID VARCHAR( 256 ) NOT NULL ,
  How VARCHAR( 50 ) NOT NULL DEFAULT 'chance' ,
  PRIMARY KEY ( UsageID )
) ENGINE = MYISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->usage_push (
  PushID bigint(20) NOT NULL auto_increment,
  UsageID bigint(20) NOT NULL,
  Stamp timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (PushID),
  UNIQUE  (UsageID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->print (
  PrintID BIGINT NOT NULL AUTO_INCREMENT  ,
  TrailHash VARCHAR( 256 ) NOT NULL ,
  Stamp timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  SessionID VARCHAR( 256 ) NOT NULL ,
  ButtonID SMALLINT NOT NULL DEFAULT '1',
  NodeCount BIGINT NOT NULL DEFAULT '0',
  PageCount BIGINT NOT NULL DEFAULT '1',
  SectionCount BIGINT NOT NULL DEFAULT '1',
  PRIMARY KEY ( PrintID )
) ENGINE = MYISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->print_push (
  PushID bigint(20) NOT NULL auto_increment,
  PrintID bigint(20) NOT NULL,
  Stamp timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (PushID),
  UNIQUE  (PrintID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->rel_control (
  RelControlID bigint(20) NOT NULL auto_increment,
  RelationshipID bigint(20) NOT NULL,
  ControlID bigint(20) NOT NULL,
  PRIMARY KEY (RelControlID)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->control (
  ControlID bigint(20) NOT NULL auto_increment,
  Term varchar(15) NOT NULL,
  Name varchar(100) DEFAULT NULL,
  Image varchar(50) NOT NULL,
  isForward tinyint(1) NOT NULL DEFAULT '0',
  Implies bigint(20) DEFAULT NULL,
  PRIMARY KEY (ControlID)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=9 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->old_slug (
  OldSlugID bigint(20) NOT NULL AUTO_INCREMENT,
  Slug varchar(100) NOT NULL,
  Hash varchar(50) NOT NULL,
  PRIMARY KEY (OldSlugID),
  UNIQUE KEY Slug (Slug)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->high_crunch (
  CrunchID bigint(20) NOT NULL AUTO_INCREMENT,
  TrailHash varchar(50) NOT NULL,
  Followers bigint(20) NOT NULL,
  Prints bigint(20) NOT NULL,
  AvgDepth float NOT NULL,
  AvgDefDepth float NOT NULL,
  Stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (CrunchID),
  UNIQUE KEY TrailHash (TrailHash)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

$sql = "CREATE TABLE $wt_tables->mid_crunch (
  CrunchID bigint(20) NOT NULL AUTO_INCREMENT,
  TrailHash varchar(50) NOT NULL,
  SessionID varchar(256) NOT NULL,
  Hits bigint(20) NOT NULL,
  DefaultOnlyHits bigint(20) NOT NULL,
  EntryHash varchar(50) NOT NULL,
  ExitHash varchar(50) NOT NULL,
  Prints int(11) NOT NULL,
  Stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (CrunchID),
  UNIQUE KEY TrailSession (TrailHash,SessionID)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;";
dbDelta($sql);

?>