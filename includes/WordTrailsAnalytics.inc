<?php
class WordTrailsAnalytics {
    const WEV = "WEV";
    const BEV = "BEV";
    const INDEX = "index";
    const DIRECT = "direct";
    const INDIRECT = "intersection";
    const CHANCE = "chance";

    const USAGE_INSERT = "INSERT INTO %t (UsageID, Stamp, Type, TrailHash, NodeHash, SessionID, How) VALUES (NULL, %s, %s, %s, %s, %s, %s)";
    const PRINT_INSERT = "INSERT INTO %t (PrintID,TrailHash, Stamp, SessionID, ButtonID, NodeCount, PageCount, SectionCount) VALUES (NULL, %s, %s, %s, %d, %s, %s, %s)";

    const PULL_BY_HASH = "SELECT * FROM %t WHERE TrailHash IN (%s) ORDER BY Stamp ASC";
    const PULL_BY_SID = "SELECT * FROM %t WHERE SessionID = %s ORDER BY Stamp ASC";
    const PULL_BY_HASH_AND_SID = "SELECT * FROM %t WHERE TrailHash = %s AND SessionID = %s ORDER BY Stamp ASC";
    const PULL_BY_HASH_AND_SIDS = "SELECT * FROM %t WHERE TrailHash = '%hash%' AND SessionID IN (%sids%) ORDER BY SessionID, Stamp ASC";
    const PULL_BY_SIDS = "SELECT * FROM %t WHERE SessionID IN (%sids%) ORDER BY Stamp ASC";
    
    const PULL_HASHES_FOR_SID = "SELECT DISTINCT TrailHash FROM %t WHERE SessionID = %s";

    const PULL_FOR_TRANSMIT = "SELECT * FROM %t WHERE %s NOT IN (SELECT %s FROM %p)";
    
    const PULL_FOLLOW_COUNTS = "SELECT TrailHash, COUNT(DISTINCT SessionID) as Follows FROM %t WHERE TrailHash IN(%s) GROUP BY TrailHash";
    
    const SESSIONS_ALREADY_CRUNCHED = "SELECT DISTINCT SessionID FROM %t WHERE TrailHash = '%hash%'";
    const NONCRUNCHED_SESSIONS = "SELECT DISTINCT SessionID, MAX(Stamp) as LastStamp FROM %t WHERE TrailHash ='%hash%' AND SessionID NOT IN (%crunched%) GROUP BY SessionID ORDER BY LastStamp ASC";
    const SESSIONS_IN_INTERVAL = "SELECT DISTINCT uc.SessionID FROM (%uncrunched%) as uc WHERE 1=1";
    
    const ALL_CRUNCHABLE_SESSIONS_FOR_TRAIL =  "SELECT DISTINCT uc.SessionID FROM (SELECT SessionID, MAX(Stamp) as LastStamp FROM %t WHERE TrailHash ='%hash%' GROUP BY SessionID ORDER BY LastStamp ASC) as uc WHERE uc.LastStamp < DATE_SUB(NOW(), INTERVAL 1 DAY)";

    const SELECT_MID_CRUNCH = "SELECT TrailHash, SessionID, Hits, DefaultOnlyHits, EntryHash, ExitHash, Prints, Stamp FROM %t WHERE TrailHash IN (%s)";
    const INSERT_MID_CRUNCHED = "INSERT INTO %t (CrunchID, TrailHash, SessionID, Hits, DefaultOnlyHits, EntryHash, ExitHash, Prints, Stamp) VALUES ";
    const INSERT_MID_CRUNCHED_VALUES = "(NULL, %s, %s, %d, %d, %s, %s, %d, %s)";
    const INSERT_MID_CRUNCHED_DUPLICATE = " ON DUPLICATE KEY UPDATE CrunchID = LAST_INSERT_ID(CrunchID), TrailHash = VALUES(TrailHash), SessionID = VALUES(SessionID), Hits = VALUES(Hits), DefaultOnlyHits = VALUES(DefaultOnlyHits), EntryHash = VALUES(EntryHash), ExitHash = VALUES(ExitHash), Prints = VALUES(Prints), Stamp = VALUES(Stamp)";

    const SELECT_HIGH_CRUNCH = "SELECT TrailHash, Followers, Prints, AvgDepth, AvgDefDepth, Stamp FROM %t WHERE TrailHash IN (%s)";
    const INSERT_HIGH_CRUNCH = "INSERT INTO %t (CrunchID, TrailHash, Followers, Prints, AvgDepth, AvgDefDepth, Stamp) VALUES (NULL, %s, %d, %d, %f, %f, %s) ON DUPLICATE KEY UPDATE CrunchID = VALUES(CrunchID), TrailHash = VALUES(TrailHash), Followers = VALUES(Followers), Prints = VALUES(Prints), AvgDepth = VALUES(AvgDepth), AvgDefDepth = VALUES(AvgDefDepth), Stamp = VALUES(Stamp)";
    
    const PULL_NONCRUNCHED_SESSION_COUNT = "SELECT COUNT(DISTINCT SessionID) FROM %ua% AS ua WHERE TrailHash IN(%hashes%) AND Stamp < DATE_SUB(NOW(), INTERVAL 1 DAY) AND SessionID NOT IN (SELECT SessionID FROM %mc% As mc WHERE mc.TrailHash = ua.TrailHash) GROUP BY TrailHash";
    const PULL_NONCRUNCHED_SESSIONS_FOR_SINGLE_TRAIL = "SELECT DISTINCT SessionID FROM %ua% AS ua WHERE TrailHash = '%hash%'%stamplimits% AND SessionID NOT IN (SELECT SessionID FROM %mc% AS mc WHERE mc.TrailHash = ua.TrailHash) GROUP BY SessionID";
    const PULL_NONCRUNCHED_SESSION_TRAIL_COMBO = "SELECT DISTINCT SessionID, TrailHash FROM %ua% AS ua WHERE TrailHash IN(%hashes%) AND Stamp < DATE_SUB(NOW(), INTERVAL 1 DAY) AND SessionID NOT IN (SELECT SessionID FROM %mc% As mc WHERE mc.TrailHash = ua.TrailHash) GROUP BY TrailHash LIMIT 1";

    const SESSION_INTERVAL = " AND ua.Stamp < DATE_SUB(NOW(), INTERVAL 1 DAY)";
    
    private static $crunchtime = null;
    
    private static $instance;

    public $hit_list = array();
    public $print_list = array();
    //private static $hit_list = array();
    //private static $print_list = array();
    private static $HIT_DEFAULT = array("TrailHash"=>"", "NodeHash"=>null,"Stamp"=>0, "SessionID"=>"", "Type"=>self::WEV, "How"=>self::CHANCE, "Committed"=>false);
    private static $PRINT_DEFAULT = array("TrailHash"=>"", "Stamp"=>0, "SessionID"=>"", "ButtonID"=>1, "NodeCount"=>0, "PageCount"=>1, "SectionCount"=>1);
    private static $TRAIL_CRUNCH = array("TrailHash"=>"","TrailName"=>"","Followers"=>0,"Prints"=>0,"Hits"=>null,"TrailCount"=>0,"DefaultOnlyCount"=>0,"AvgDepth"=>0,"AvgDefaultDepth"=>0,"PercentFollowed"=>0, "PercentDefaultFollowed"=>0);
    private static $SESSION_CRUNCH = array("Prints"=>0,"WEVHits"=>0,"BEVHits"=>0,"indexHits"=>0,"DefaultOnlyHits"=>0,"EnterHit"=>null,"LastHit"=>null,"LastStamp"=>null);

    public static $print_buttons = array("Unknown", "WEV", "BEV", "Trail Index");
    
    public $followcounts = array();

    public function __construct() {
	self::$instance = &$this;
    }
    public function __wakeup() {
	self::$instance = &$this;
    }
    public function __sleep() {
	return array_keys(array_diff(get_object_vars($this), array("followcounts")));
    }
    
    public static function counts($trailhash) {
	if (empty(self::$instance->followcounts)) self::updateFollowCounts();
	if (isset(self::$instance->followcounts[$trailhash])) return (int)self::$instance->followcounts[$trailhash];
	return 0;
    }
    private static function checkInstance() {
	if (!is_object(self::$instance))
	    WordTrailsGlobal::makeInstance();
    }
    
    private static function updateFollowCounts() {
	global $wpdb;
	$trails = WordTrailsGlobal::getAllTrailHashes();
	$follow_results = $wpdb->get_results(str_replace("%s", implode(",",array_map(array("WordTrailsUtilities", "wrap_with_quotes"),$trails)), str_replace("%t", WordTrailsGlobal::$tables->usage, self::PULL_FOLLOW_COUNTS)), OBJECT);
	self::checkInstance();
	self::$instance->followcounts = array();
	foreach ($follow_results as $follow) {
	    self::$instance->followcounts[$follow->TrailHash] = $follow->Follows;
	}
    }

    public static function hit($TrailHash, $NodeHash=null, $Type=self::WEV, $How=self::CHANCE){
	$thisHit = array_merge(self::$HIT_DEFAULT, array("TrailHash"=>$TrailHash, "NodeHash"=>$NodeHash, "Type"=>$Type, "How"=>$How));
	self::checkInstance();
	$lastStamp = self::$instance->hit_list[max(0,count(self::$instance->hit_list)-1)]["Stamp"];
	if (is_null($NodeHash)) {
	    if (false !== self::array_multi_search(array("Stamp"=>$lastStamp, "TrailHash"=>$TrailHash, "Type"=>$Type), self::$instance->hit_list)) return false;
	} else {
	    if (false !== self::array_multi_search(array("Stamp"=>$lastStamp, "NodeHash"=>$NodeHash), self::$instance->hit_list)) return false;
	}
	$thisHit["Stamp"] = date("Y-m-d H:i:s");
	$thisHit["SessionID"] = session_id();
	self::$instance->hit_list[] = $thisHit;
	if ($How == self::DIRECT) self::pushHitsForTrail($TrailHash);
    }

    private static function pushHitsForTrail($TrailHash) {
	$alsoStore = &self::array_multi_search(array("TrailHash"=>$TrailHash, "Committed"=>false), self::$instance->hit_list);
	if (is_array($alsoStore) && !empty($alsoStore)) {
	    foreach ($alsoStore as $id => &$hit) {
		 self::pushHitToDB($hit);
	    }
	}
    }
    private static function pushHitToDB($HitArray) {
	global $wpdb;
	$wpdb->query(
	    str_ireplace(
		array("'null'", '"null"'),
		"NULL",
		$wpdb->prepare(
		    str_replace(
			"%t",
			WordTrailsGlobal::$tables->usage,
			self::USAGE_INSERT
		    ),
		    $HitArray["Stamp"],
		    $HitArray["Type"],
		    $HitArray["TrailHash"],
		    (is_null($HitArray["NodeHash"]) ? "null" : $HitArray["NodeHash"]),
		    $HitArray["SessionID"],
		    $HitArray["How"]
		)
	    )
	);
	$hits = &self::array_multi_search(array("TrailHash"=>$HitArray["TrailHash"], "NodeHash"=>$HitArray["NodeHash"], "Stamp"=>$HitArray["Stamp"]), self::$instance->hit_list);
	if (is_array($hits) && !empty($hits)) {
	    foreach ($hits as $key => &$hit) {
		$hit["Committed"] = true;
	    }
	}
    }

    public static function pdf($TrailHash, $ButtonID = null, $NodeCount=null, $PageCount=null, $SectionCount=null) {
	global $wpdb;
	$thisPrint = array_merge(self::$PRINT_DEFAULT, array("TrailHash" => $TrailHash, "ButtonID"=>$ButtonID, "NodeCount"=>$NodeCount, "PageCount"=>$PageCount, "SectionCount"=>$SectionCount));
	$thisPrint["Stamp"] = date("Y-m-d H:i:s");
	$thisPrint["SessionID"] = session_id();
	self::$instance->print_list[] = $thisPrint;
	$wpdb->query(
	    $wpdb->prepare(
		str_replace(
		    "%t",
		    WordTrailsGlobal::$tables->print,
		    self::PRINT_INSERT
		),
		$thisPrint["TrailHash"],
		$thisPrint["Stamp"],
		$thisPrint["SessionID"],
		(int)$thisPrint["ButtonID"],
		$thisPrint["NodeCount"],
		$thisPrint["PageCount"],
		$thisPrint["SectionCount"]
	    )
	);
	self::pushHitsForTrail($TrailHash);
    }

    public static function array_multi_search($needlestack, &$haystack){
	$results = array();
	foreach ($haystack as $id => &$needle) {
	    if (!is_array($needle)) continue;
	    $match = true;
	    foreach ($needlestack as $key=>$val) {
		if (!array_key_exists($key, $needle)) {
		    $match = false;
		    break;
		}
		if ($needle[$key] !== $val) {
		    $match = false;
		    break;
		}
	    }
	    if ($match) $results[$id] = &$needle;
	}
	return (empty($results) ? false : $results);
    }
    
    public static function pullAllSessionData($sid) {
	$hashes = self::trailsFromSession($sid);
	$analytics = array();
	if (!empty($hashes)) {
	    usort($hashes, array("WordTrailsNode","nsort_name"));
	    foreach ($hashes as $hash) {
		self::pullDetailsFor($hash, $sid, $analytics);
	    }
	}
	return $analytics;
    }
    
    public static function trailsFromSession($sid) {
	global $wpdb;
	$usql = str_replace("%t", WordTrailsGlobal::$tables->usage, self::PULL_HASHES_FOR_SID);
	$psql = str_replace("%t", WordTrailsGlobal::$tables->print, self::PULL_HASHES_FOR_SID);
	$uhashes = $wpdb->get_col($wpdb->prepare($usql, $sid));
	$phashes = $wpdb->get_col($wpdb->prepare($psql, $sid));
	$hashes = array_unique(array_merge($uhashes, $phashes));
	unset($uhashes, $phashes);
	if (!empty($hashes)) {
	    usort($hashes, array("WordTrailsNode","nsort_name"));
	}
	return $hashes;
    }
    
    public static function pullDetailsFor($trail, $sid, &$analytics = array()) {
	global $wpdb;
	$sql = self::PULL_BY_HASH_AND_SID;
	$usql = str_replace("%t", WordTrailsGlobal::$tables->usage, $sql);
	$psql = str_replace("%t", WordTrailsGlobal::$tables->print, $sql);
	//trigger_error("\t".$wpdb->prepare($usql, $trail, $sid));
	$uresults = $wpdb->get_results($wpdb->prepare($usql, $trail, $sid), OBJECT);
	$presults = $wpdb->get_results($wpdb->prepare($psql, $trail, $sid), OBJECT);
	foreach ($uresults as $result) {
	    if (!is_array($analytics[$trail])) $analytics[$trail] = array();
	    if (!is_array($analytics[$trail][$sid])) $analytics[$trail][$sid] = array();
	    if (!is_array($analytics[$trail][$sid]["hit"])) $analytics[$trail][$sid]["hit"] = array();
	    array_push($analytics[$trail][$sid]["hit"], $result);
	}
	unset($uresults);
	
	foreach ($presults as $result) {
	    if (!is_array($analytics[$trail])) $analytics[$trail] = array();
	    if (!is_array($analytics[$trail][$sid])) $analytics[$trail][$sid] = array();
	    if (!is_array($analytics[$trail][$sid]["print"])) $analytics[$trail][$sid]["print"] = array();
	    array_push($analytics[$trail][$sid]["print"], $result);
	}
	unset($presults);
	return $analytics;
    }
    
    public static function pullDetailsForTrailSessions($trail, $sids) {
	global $wpdb;
	//trigger_error("pull for trail sessions");
	$sql = self::PULL_BY_HASH_AND_SIDS;
	$sql = str_replace(array("%hash%", "%sids%"), array($trail, implode(",",array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $sids))), $sql);
	$usql = str_replace("%t", WordTrailsGlobal::$tables->usage, $sql);
	$psql = str_replace("%t", WordTrailsGlobal::$tables->print, $sql);
	//trigger_error("\t$usql");
	$uresults = $wpdb->get_results($usql, OBJECT);
	$analytics = array();
	foreach ($uresults as $result) {
	    if (!is_array($analytics[$trail])) $analytics[$trail] = array();
	    if (!is_array($analytics[$trail][$result->SessionID])) $analytics[$trail][$result->SessionID] = array();
	    if (!is_array($analytics[$trail][$result->SessionID]["hit"])) $analytics[$trail][$result->SessionID]["hit"] = array();
	    array_push($analytics[$trail][$result->SessionID]["hit"], $result);
	}
	unset($uresults);
	//trigger_error("\t$psql");
	$presults = $wpdb->get_results($psql, OBJECT);
	foreach ($presults as $result) {
	    if (!is_array($analytics[$trail])) $analytics[$trail] = array();
	    if (!is_array($analytics[$trail][$result->SessionID])) $analytics[$trail][$result->SessionID] = array();
	    if (!is_array($analytics[$trail][$result->SessionID]["print"])) $analytics[$trail][$result->SessionID]["print"] = array();
	    array_push($analytics[$trail][$result->SessionID]["print"], $result);
	}
	unset($presults);
	return $analytics;
    }
    
    public static function getSessionCountForTrail($trail) {
	global $wpdb;
	$utotalsql = str_replace(array("%t","%hash%"), array(WordTrailsGlobal::$tables->usage, $trail), self::ALL_CRUNCHABLE_SESSIONS_FOR_TRAIL);
	$ptotalsql = str_replace(array("%t","%hash%"), array(WordTrailsGlobal::$tables->print, $trail), self::ALL_CRUNCHABLE_SESSIONS_FOR_TRAIL);
	$utotal = $wpdb->get_col($utotalsql);
	$ptotal = $wpdb->get_col($ptotalsql);
	$total = array_unique(array_merge($utotal, $ptotal));
	return count($total);
    }
    public static function getSessionCountForTrails($trails = array()) {
	if (is_string($trails) && WordTrailsUtilities::isHash($trails)) $trails = array($trails);
	if (!is_array($trails) || empty($trails)) $trails = array_merge(array(WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION)), WordTrailsGlobal::getAllTrailHashes());
	$totals = array();
	foreach ($trails as $trail) {
	    $totals[$trail] = self::getSessionCountForTrail($trail);
	}
	return $totals;
    }
    
    public static function incrementalProgress($trail, $total = 0) {
	global $wpdb;
	if (is_null(self::$crunchtime)) self::$crunchtime = date("Y-m-d H:i:s");
	$crunchedsql = str_replace(array("%t","%hash%"), array(WordTrailsGlobal::$tables->mid_crunch, $trail), self::SESSIONS_ALREADY_CRUNCHED);
	$crunched = $wpdb->get_col($crunchedsql);
	$crunchcount = count($crunched);
	unset($crunched);
	if ($total > 0 && $crunchcount >= $total) return $total;
	$newcrunch = array();
	self::getNewMidLevelsForTrail($newcrunch, $trail, null, null, true);
	$crunchcount = $crunchcount + count($newcrunch);
	unset($newcrunch);
	//crunch trail?
	return $crunchcount;
    }
    
    public static function pullMidLevelForTrails($trails, $start = null, $end = null, $round = null) {
	global $wpdb;
	$sitehash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	if (is_string($trails) && WordTrailsUtilities::isHash($trails)) $trails = array($trails);
	if (!is_array($trails)) $trails = array_merge(array($sitehash), WordTrailsGlobal::getAllTrailHashes());
	//trigger_error("pull mid: " . implode(",",$trails) . " [$start to $end]");
	if (is_null(self::$crunchtime)) self::$crunchtime = date("Y-m-d H:i:s");
	$sql = str_replace("%t", WordTrailsGlobal::$tables->mid_crunch, self::SELECT_MID_CRUNCH);
	$sql = str_replace("%s", implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $trails)), $sql);
	if (!is_null($start) && false !== strtotime($start)) $sql .= " AND Stamp > '$start'";
	if (!is_null($end) && false !== strtotime($end)) $sql .= " AND Stamp <= '$end'";
	//if (!is_null($start) && is_int($start)) $sql .= " LIMIT $start";
	//if (!is_null($end) && is_int($end)) $sql .= ", $end";
	$trails = WordTrailsGlobal::getNode($trails);
	$mcrunch = array();
	if (is_int($round)) {
	    $rpq = 500;
	    $sql .= " LIMIT " . ($rpq*($round-1)) . ", " . $rpq;
	}
	//trigger_error("\t".$sql);
	$mcresults = $wpdb->get_results($sql, OBJECT);
	//trigger_error("\t " . count($mcresults));
	foreach ($mcresults as &$result) {
	    $hash = $result->TrailHash;
	    if (!is_object($trails[$hash]) && $hash != $sitehash) continue;
	    if (!is_array($mcrunch[$hash])) $mcrunch[$hash] = array();
	    $mcrunch[$hash][$result->SessionID] = array_merge(self::$SESSION_CRUNCH);
	    $crunch = &$mcrunch[$hash][$result->SessionID];
	    $crunch["Prints"] = $result->Prints;
	    $crunch["WEVHits"] = $result->Hits;
	    $crunch["DefaultOnlyHits"] = $result->DefaultOnlyHits;
	    $crunch["EnterHit"] = $result->EntryHash;
	    $crunch["LastHit"] = $result->ExitHash;
	    $crunch["LastStamp"] = $result->Stamp;
	    unset($result);
	}
	if (is_int($round) && count($mcresults)) {
	    unset($mcresults);
	    return array($mcrunch, true);
	}
	unset($mcresults);
	$shellcrunch = array_diff(array_keys($trails), array_keys($mcrunch));
	foreach ($shellcrunch as $hash) {
	    if (!is_object($trails[$hash]) && $hash != $sitehash) continue;
	    if (!is_array($mcrunch[$hash])) $mcrunch[$hash] = array();
	}
	foreach ($mcrunch as $hash => &$crunches) {
	    $ncrunches = array();
	    self::getNewMidLevelsForTrail($ncrunches, $hash, $start, $end);
	    $crunches = $crunches + $ncrunches;
	    @ksort($crunches);
	}
	return array($mcrunch, false);
    }
    
    public static function getNewMidLevelsForTrail(&$crunches, $hash, $start = null, $end = null, $runonce = false) {
	//trigger_error("get new mids for $hash [$start to $end]");
	global $wpdb;
	$usage = WordTrailsGlobal::$tables->usage;
	$print = WordTrailsGlobal::$tables->print;
	
	$stamplimits = "";
	if (!is_null($start) && false !== strtotime($start)) $stamplimits .= " AND ua.Stamp > '$start'";
	if (!is_null($end) && false !== strtotime($end)) $stamplimits .= " AND ua.Stamp <= '$end'";
	if (empty($stamplimits)) $stamplimits = self::SESSION_INTERVAL;
	$sessions = str_replace(array("%hash%", "%stamplimits%", "%mc%"), array($hash, $stamplimits, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSIONS_FOR_SINGLE_TRAIL);
	//else $sessions .= self::SESSION_INTERVAL;
	$trail = WordTrailsGlobal::getNode($hash);
	$defaults = array();
	if (is_object($trail)) $trail->crawl_defaults($defaults);
	$round = 1;
	$rpq = 100;
	//trigger_error("get sessions: " . str_replace("%ua%", $usage, $sessions) . " LIMIT " . ($rpq*($round-1)) . ", " . ($rpq*$round));
	//$wpdb->timer_start();
	$usessionsql = str_replace("%ua%", $usage, $sessions) . " LIMIT ";
	$usessionresult = $wpdb->get_col($usessionsql . ($rpq*($round-1)) . ", " . $rpq);
	//trigger_error($wpdb->timer_stop());
	$mergedsessions = array_values($usessionresult);
	unset($usessionresult);
	$wpdb->flush();
	
	$ucrunches = array();
	while (!empty($mergedsessions)) {
	    $analytics = self::pullDetailsForTrailSessions($hash, $mergedsessions);
	    foreach ($mergedsessions as $session) {
		self::getNewMidLevel($ucrunches, $hash, $session, $defaults, $analytics);
	    }
	    if ($runonce || count($ucrunches) >= 100) {
		self::updateMidLevel($ucrunches, $hash);
		$crunches = array_merge($crunches, $ucrunches);
		unset($ucrunches);
		$ucrunches = array();
		if ($runonce) return;
	    }
	    $usessionresult = $wpdb->get_col($usessionsql . ($rpq*($round-1)) . ", " . $rpq);
	    $mergedsessions = array_values($usessionresult);
	    unset($usessionresult);
	    $wpdb->flush();
	    $round++;
	}
	if (!empty($ucrunches)) {
	    self::updateMidLevel($ucrunches, $hash);
	    $crunches = array_merge($crunches, $ucrunches);
	    unset($ucrunches);
	}
	
	$pcrunches = array();
	$round = 1;
	$rpq = 100;
	//trigger_error("get print sessions: " . str_replace("%ua%", $print, $sessions) . " LIMIT " . ($rpq*($round-1)) . ", " . ($rpq*$round));
	//$wpdb->timer_start();
	$psessionsql = str_replace("%ua%", $print, $sessions) . " LIMIT ";
	$psessionresult = $wpdb->get_col($psessionsql . ($rpq*($round-1)) . ", " . $rpq);
	//trigger_error($wpdb->timer_stop());
	$mergedsessions = array_values($psessionresult);
	unset($psessionresult);
	$wpdb->flush();
	while (!empty($mergedsessions)) {
	    $analytics = self::pullDetailsForTrailSessions($hash, $mergedsessions);
	    foreach ($mergedsessions as $session) {
		self::getNewMidLevel($pcrunches, $hash, $session, $defaults, $analytics);
	    }
	    if ($runonce || count($pcrunches) >= 100) {
		self::updateMidLevel($pcrunches, $hash);
		$crunches = array_merge($crunches, $pcrunches);
		unset($pcrunches);
		$pcrunches = array();
		if ($runonce) return;
	    }
	    $round++;
	    $psessionresult = $wpdb->get_col($psessionsql . ($rpq*($round-1)) . ", " . $rpq);
	    $mergedsessions = array_values($psessionresult);
	    unset($psessionresult);
	    $wpdb->flush();
	}
	if (!empty($pcrunches)) {
	    self::updateMidLevel($pcrunches, $hash);
	    $crunches = array_merge($crunches, $pcrunches);
	    unset($pcrunches);
	}
    }
    
    private static function getNewMidLevel(&$crunches, $hash, $sid, $defaults = array(), &$analytics = array()) {
	//trigger_error("get new mid for $hash and $sid");
	if (empty($analytics))	$analytics = self::pullDetailsFor($hash, $sid);
	//trigger_error("\tgot details");
	if (is_array($analytics[$hash][$sid])) {
	    if (empty($defaults)) {
		$trail = WordTrailsGlobal::getNode($hash);
		if (is_object($trail)) $trail->crawl_defaults($defaults);
	    }
	    $hits = &$analytics[$hash][$sid]["hit"];
	    $prints = &$analytics[$hash][$sid]["print"];
	    $crunch = array_merge(self::$SESSION_CRUNCH);
	    if (is_array($hits)) {
		$wev = array();
		$defwev = array();
		foreach ($hits as $stamp => $hit) {
		    switch ($hit->Type) {
			case "WEV":
			    if (is_null($hit->NodeHash) || empty($hit->NodeHash)) continue;
			    array_push($wev, $hit->NodeHash);
			    if (in_array($hit->NodeHash, $defaults)) array_push($defwev, $hit->NodeHash);
			    if (is_null($crunch["EnterHit"])) $crunch["EnterHit"] = $hit->NodeHash;
			    $crunch["LastHit"] = $hit->NodeHash;
			    break;
			case "BEV":
			    $crunch["BEVHits"] = $crunch["BEVHits"] + 1;
			    $str = "Trail Map";
			    if (is_null($crunch["EnterHit"])) $crunch["EnterHit"] = $str;
			    $crunch["LastHit"] = $str;
			    break;
			case "index":
			    $str = "Trail Index";
			    $crunch["indexHits"] = $crunch["indexHits"] + 1;
			    if (is_null($crunch["EnterHit"])) $crunch["EnterHit"] = $str;
			    $crunch["LastHit"] = $str;
			    array_push($wev, $hit->UsageID);
			    array_push($defwev, $hit->UsageID);
			    break;
		    }
		    $crunch["LastStamp"] = $hit->Stamp;
		}
		$crunch["WEVHits"] = count(array_unique($wev));
		$crunch["DefaultOnlyHits"] = count(array_unique($defwev));
	    }
	    unset($hits);
	    //trigger_error("\tfinished hit crunch");
	    if (is_array($prints)) {
		foreach ($prints as $print) {
		    $crunch["Prints"] = $crunch["Prints"] + 1;
		    if (is_null($crunch["EnterHit"])) {
			$crunch["EnterHit"] = "<i>No Hits, Print Only</i>";
			$crunch["LastHit"] = "<i>No Hits, Print Only</i>";
			$crunch["LastStamp"] = $print->Stamp;
		    }
		}
	    }
	    unset($prints);
	    unset($analytics[$hash][$sid]);
	    //trigger_error("\tfinished print crunch");
	    $crunches[$sid] = $crunch;
	    if (defined("DOING_CRON") && DOING_CRON) return self::updateMidLevel($crunches, $hash);
	}
    }
    
    public static function updateMidLevel(&$crunches, $hash) {
	//trigger_error("update mids for $hash: " . count($crunches));
	global $wpdb;
	$wpdb->flush();
	if (is_null(self::$crunchtime)) self::$crunchtime = date("Y-m-d H:i:s");
	$start = str_replace("%t", WordTrailsGlobal::$tables->mid_crunch, self::INSERT_MID_CRUNCHED);
	//$siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	$count = 0;
	$sids = array();
	$storeprecrunchedsql = "" . $start;
	foreach ($crunches as $sid => $crunch) {
	    if (strtotime(self::$crunchtime) - strtotime($crunch["LastStamp"]) < 24*60*60) continue;
	    $storeprecrunchedsql .= $wpdb->prepare(self::INSERT_MID_CRUNCHED_VALUES,
						   $hash,
						   $sid,
						   $crunch["WEVHits"],
						   $crunch["DefaultOnlyHits"],
						   $crunch["EnterHit"],
						   $crunch["LastHit"],
						   $crunch["Prints"],
						   $crunch["LastStamp"]
						  );
	    array_push($sids, $sid);
	    $count++;
	    if ($count >= 50) {
		$storeprecrunchedsql .= self::INSERT_MID_CRUNCHED_DUPLICATE;
		//trigger_error($storeprecrunchedsql);
		$result = $wpdb->query($storeprecrunchedsql);
		$sids = array();
		$wpdb->flush();
		$storeprecrunchedsql = $start;
		$count = 0;
	    } else {
		$storeprecrunchedsql .= ", ";
	    }
	}
	if (strlen($storeprecrunchedsql) > strlen($start)) {
	    $storeprecrunchedsql = substr($storeprecrunchedsql, 0, -2);
	    $storeprecrunchedsql .= self::INSERT_MID_CRUNCHED_DUPLICATE;
	    $result = $wpdb->query($storeprecrunchedsql);
	    $wpdb->flush();
	}
    }
    
    public static function pullHighLevelForTrails($trails = array()){
	global $wpdb;
	$sitehash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	if (is_string($trails) && WordTrailsUtilities::isHash($trails)) $trails = array($trails);
	if (!is_array($trails) || empty($trails)) {
	    $trails = array_merge(array($sitehash), WordTrailsGlobal::getAllTrailHashes());
	}
	self::$crunchtime = date("Y-m-d H:i:s");
	$sql = str_replace("%t", WordTrailsGlobal::$tables->high_crunch, self::SELECT_HIGH_CRUNCH);
	$sql = str_replace("%s", implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $trails)), $sql);
	$hcresults = $wpdb->get_results($sql, OBJECT_K);
	$trails = WordTrailsGlobal::getNode($trails);
	$hcrunch = array();
	if (!is_array($hcresults)) $hcresults = array();
	foreach ($hcresults as $hash => &$result) {
	    $trail = &$trails[$hash];
	    if (!is_object($trail) && $hash != $sitehash) continue;
	    $hcrunch[$hash] = array_merge(self::$TRAIL_CRUNCH);
	    $hcrunch[$hash]["TrailHash"] = $hash;
	    $hcrunch[$hash]["Followers"] = $result->Followers;
	    $hcrunch[$hash]["Prints"] = $result->Prints;
	    if (is_object($trail)) {
		$defaults = array();
		$trail->crawl_defaults($defaults);
		$hcrunch[$hash]["TrailName"] = $trail->getName();
		$hcrunch[$hash]["TrailCount"] = count($trail->child_hashes);
		$hcrunch[$hash]["DefaultOnlyCount"] = count($defaults);
		unset($defaults);
	    } elseif ($hash == $sitehash) {
		$hcrunch[$hash]["TrailName"] = "<i>{Trail Index}</i>";
		$hcrunch[$hash]["TrailCount"] = 1;
		$hcrunch[$hash]["DefaultOnlyCount"] = 1;
	    }
	    $hcrunch[$hash]["AvgDepth"] = $result->AvgDepth;
	    $hcrunch[$hash]["AvgDefaultDepth"] = $result->AvgDefDepth;
	    if ($hcrunch[$hash]["TrailCount"] == 0) {
		$hcrunch[$hash]["PercentFollowed"] = 0;   
	    } else {
		$hcrunch[$hash]["PercentFollowed"] = round(100*($hcrunch[$hash]["AvgDepth"]/$hcrunch[$hash]["TrailCount"]),1);
	    }
	    if ($hcrunch[$hash]["DefaultOnlyCount"] == 0) {
		$hcrunch[$hash]["PercentDefaultFollowed"] = 0;
	    } else {
		$hcrunch[$hash]["PercentDefaultFollowed"] = round(100*($hcrunch[$hash]["AvgDefaultDepth"]/$hcrunch[$hash]["DefaultOnlyCount"]),1);
	    }
	    $hcrunch[$hash]["Stamp"] = $result->Stamp;
	    unset($result);
	}
	unset($hcresults);
	$shellcrunch = array_diff(array_keys($trails), array_keys($hcrunch));
	foreach ($shellcrunch as $hash) {
	    $trail = &$trails[$hash];
	    if (!is_object($trail) && $hash != $sitehash) continue;
	    $hcrunch[$hash] = array_merge(self::$TRAIL_CRUNCH);
	    $hcrunch[$hash]["TrailHash"] = $hash;
	    if (is_object($trail)) {
		$hcrunch[$hash]["TrailName"] = $trail->getName();
		$hcrunch[$hash]["TrailCount"] = count($trail->child_hashes);
		$defaults = array();
		$trail->crawl_defaults($defaults);
		$hcrunch[$hash]["DefaultOnlyCount"] = count($defaults);
		unset($defaults);
	    } elseif ($hash == $sitehash) {
		$hcrunch[$hash]["TrailName"] = "<i>{Trail Index}</i>";
		$hcrunch[$hash]["TrailCount"] = 1;
		$hcrunch[$hash]["DefaultOnlyCount"] = 1;
	    }
	}
	foreach ($hcrunch as $hash => &$crunch) {
	    $updated = self::getNewHighLevel($crunch);
	    if (defined("DOING_CRON") && DOING_CRON) return $updated;
	}
	return $hcrunch;
    }
    
    public static function getNewHighLevel(&$crunch) {
	if (is_null(self::$crunchtime)) self::$crunchtime = date("Y-m-d H:i:s");
	$midcrunches = array();
	self::getNewMidLevelsForTrail($midcrunches, $crunch["TrailHash"]);
	unset($midcrunches);
	$repeat = true;
	$round = 1;
	while ($repeat) {
	    list($midlevels, $repeat) = self::pullMidLevelForTrails($crunch["TrailHash"], $crunch["Stamp"], self::$crunchtime, $round);
	    if (defined("DOING_CRON") && DOING_CRON && empty($midlevels[$crunch["TrailHash"]])) return false;
	    self::reCrunchHighLevel($crunch, $midlevels[$crunch["TrailHash"]]);
	    unset($midlevels);
	    $round++;
	}
	self::updateHighLevel($crunch);
	if (defined("DOING_CRON") && DOING_CRON) return true;
	$repeat = true;
	$round = 1;
	while ($repeat) {
	    list($midlevels, $repeat) = self::pullMidLevelForTrails($crunch["TrailHash"], self::$crunchtime, null, $round);
	    if (defined("DOING_CRON") && DOING_CRON && empty($midlevels[$crunch["TrailHash"]])) return false;
	    self::reCrunchHighLevel($crunch, $midlevels[$crunch["TrailHash"]]);
	    unset($midlevels);
	    $round++;
	}
    }
    
    public static function reCrunchHighLevel(&$crunch, &$midlevels = array()) {
	if (empty($midlevels)) return;
	$depthsum = $crunch["AvgDepth"] * $crunch["Followers"];
	$defdepthsum = $crunch["AvgDefaultDepth"] * $crunch["Followers"];
	$depthsum += WordTrailsUtilities::array_sum_key($midlevels, "WEVHits");
	$defdepthsum += WordTrailsUtilities::array_sum_key($midlevels, "DefaultOnlyHits");
	$crunch["Followers"] += WordTrailsUtilities::array_count_key($midlevels, "WEVHits");
	$crunch["Prints"] += WordTrailsUtilities::array_sum_key($midlevels, "Prints");
	if ($crunch["Followers"] == 0) {
	    $crunch["AvgDepth"] = 0;
	    $crunch["AvgDeafultDepth"] = 0;
	} else {
	    $crunch["AvgDepth"] = round($depthsum/$crunch["Followers"], 2);
	    $crunch["AvgDefaultDepth"] = round($defdepthsum/$crunch["Followers"], 2);
	}
	if ($crunch["TrailCount"] == 0) {
	    $crunch["PercentFollowed"] = 0;   
	} else {
	    $crunch["PercentFollowed"] = round(100*($crunch["AvgDepth"]/$crunch["TrailCount"]),1);
	}
	if ($crunch["DefaultOnlyCount"] == 0) {
	    $crunch["PercentDefaultFollowed"] = 0;
	} else {
	    $crunch["PercentDefaultFollowed"] = round(100*($crunch["AvgDefaultDepth"]/$crunch["DefaultOnlyCount"]),1);
	}
    }
    
    public static function updateHighLevel(&$crunch) {
	global $wpdb;
	if (is_null(self::$crunchtime)) self::$instance = date("Y-m-d H:i:s");
	$sql = str_replace("%t", WordTrailsGlobal::$tables->high_crunch, self::INSERT_HIGH_CRUNCH);
	$sql = $wpdb->prepare($sql,
			      $crunch["TrailHash"],
			      $crunch["Followers"],
			      $crunch["Prints"],
			      $crunch["AvgDepth"],
			      $crunch["AvgDefaultDepth"],
			      self::$crunchtime);
	$wpdb->query($sql);
    }

    public static function pullAnalyticsToTransmit() {
	global $wpdb;
	$psql = str_replace(array("%t", "%p", "%s"), array(WordTrailsGlobal::$tables->print, WordTrailsGlobal::$tables->print_push, "PrintID"), self::PULL_FOR_TRANSMIT);
	//$psql = str_replace("%s", $psql);
	$usql = str_replace(array("%t", "%t"), array(WordTrailsGlobal::$tables->usage, WordTrailsGlobal::$tables->usage_push), self::PULL_FOR_TRANSMIT);
	$usql = str_replace("%s", "UsageID", $psql);

	$results = array();
	$wpdb->show_errors();
	$presults = $wpdb->get_results($psql);
	$results["print"] = $presults;
	//$uresults = $wpdb->get_results($usql);
	//$results["hit"] = $uresults;
	return $results;
    }
    
    public static function uncrunchedCount($greaterthan = null) {
	global $wpdb;
	$hashes = WordTrailsGlobal::getAllTrailHashes();
	array_push($hashes, WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION));
	$hashstr = implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $hashes));
	$count = 0;
	$sql = str_replace(array("%ua%", "%hashes%", "%mc%"), array(WordTrailsGlobal::$tables->usage, $hashstr, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSION_COUNT);
	$result = $wpdb->get_var($sql);
	$count += (int)$result;
	$sql = str_replace(array("%ua%", "%hashes%", "%mc%"), array(WordTrailsGlobal::$tables->print, $hashstr, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSION_COUNT);
	$result = $wpdb->get_var($sql);
	$count += (int)$result;
	$wpdb->flush();
	return $count;
    }
    
    public static function uncrunchedGreaterThan($greaterthan = 100) {
	global $wpdb;
	$hashes = WordTrailsGlobal::getAllTrailHashes();
	array_push($hashes, WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION));
	$hashstr = implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $hashes));
	$count = 0;
	$sql = str_replace(array("%ua%", "%hashes%", "%mc%"), array(WordTrailsGlobal::$tables->usage, $hashstr, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSION_COUNT);
	//trigger_error($sql);
	//$wpdb->timer_start();
	$result = $wpdb->get_var($sql);
	//trigger_error($wpdb->timer_stop());
	$count += (int)$result;
	if ($count > $greaterthan) return true;
	$sql = str_replace(array("%ua%", "%hashes%", "%mc%"), array(WordTrailsGlobal::$tables->print, $hashstr, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSION_COUNT);
	//trigger_error($sql);
	//$wpdb->timer_start();
	$result = $wpdb->get_var($sql);
	//trigger_error($wpdb->timer_stop());
	$count += (int)$result;
	$wpdb->flush();
	if ($count > $greaterthan) return true;
	return false;
    }
    
    public static function cronCrunch() {
	global $wpdb;
	$wpdb->timer_start();
	$crunches = array();
	WordTrailsUtilities::setStatics();
	$hashes = WordTrailsGlobal::getAllTrailHashes();
	array_push($hashes, WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION));
	//$hashstr = implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $hashes));
	$sql = str_replace(array("%ua%", "%mc%", "%stamplimits%"), array(WordTrailsGlobal::$tables->usage, WordTrailsGlobal::$tables->mid_crunch, self::SESSION_INTERVAL), self::PULL_NONCRUNCHED_SESSIONS_FOR_SINGLE_TRAIL);
	foreach ($hashes as $hash) {
	    //trigger_error("\t" . str_replace("%hash%", $hash, $sql) . " LIMIT 1");
	    $sid = $wpdb->get_var(str_replace("%hash%", $hash, $sql) . " LIMIT 1");
	    if ($sid) {
		self::getNewMidLevel($crunches, $hash, $sid);
		WordTrailsUtilities::reScheduleCronCrunch();
		error_log("cron crunched in " . $wpdb->timer_stop() . "s");
		return;
	    }
	}
	$sql = str_replace(array("%ua%", "%mc%", "%stamplimits%"), array(WordTrailsGlobal::$tables->print, WordTrailsGlobal::$tables->mid_crunch, self::SESSION_INTERVAL), self::PULL_NONCRUNCHED_SESSIONS_FOR_SINGLE_TRAIL);
	foreach ($hashes as $hash) {
	    //trigger_error("\t" . str_replace("%hash%", $hash, $sql) . " LIMIT 1");
	    $sid = $wpdb->get_var(str_replace("%hash%", $hash, $sql) . " LIMIT 1");
	    if ($sid) {
		self::getNewMidLevel($crunches, $hash, $sid);
		WordTrailsUtilities::reScheduleCronCrunch();
		error_log("cron crunched in " . $wpdb->timer_stop() . "s");
		return;
	    }
	}
	error_log("nothing to crunch: " . $wpdb->timer_stop());
	/*
	$sql = str_replace(array("%ua%", "%hashes%", "%mc%"), array(WordTrailsGlobal::$tables->print, $hashstr, WordTrailsGlobal::$tables->mid_crunch), self::PULL_NONCRUNCHED_SESSION_TRAIL_COMBO);
	$result = $wpdb->get_row($sql, OBJECT);
	if (!empty($result)) {
	    self::getNewMidLevel($crunches, $result->TrailHash, $result->SessionID);
	    WordTrailsUtilities::reScheduleCronCrunch();
	    return;
	}
	*/
    }
}
?>