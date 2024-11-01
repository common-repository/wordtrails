<?php
class WordTrailsControlManager {
    private $controls = null;
    public function __construct() {
        $this->loadFromDB();
    }

    public function __get($name) {
        if (is_numeric($name)) return $this->fromID($name);
        return $this->fromTerm($name);
    }

    public function __sleep() {
        return array_keys(get_object_vars($this));
    }

    private function loadFromDB() {
        global $wpdb;
        $results = $wpdb->get_results("SELECT * FROM " . WordTrailsGlobal::$tables->control, OBJECT);
        if (empty($results)) {
            WordTrailsUtilities::pushDefaultControls();
            $results = $wpdb->get_results("SELECT * FROM " . WordTrailsGlobal::$tables->control, OBJECT);
        }
        $this->controls = array();
        foreach ($results as $result) {
            $this->controls[$result->ControlID] = $result;
        }
    }

    private function fromID($ID) {
        if (!is_array($this->controls)) $this->loadFromDB();
        $ID = (int)$ID;
        foreach ($this->controls as $control) {
            if ($control->ControlID == $ID) return $control;
        }
        return false;
    }

    public function fromTerm($name = "auto", $forceBackwards = false) {
        if (!is_array($this->controls)) $this->loadFromDB();
        if ($name == "") $name = "auto";
        foreach ($this->controls as $control) {
            if ($forceBackwards && $control->isForward != 0) continue;
            if ($control->Term == "auto") $backup = $control;
            if ($control->Term == $name) return $control;
        }
        return $backup;
    }

    public function getNextArrow($control, $isDefault = false) {
        return $this->getArrowFromControlPair($control, true, $isDefault);
    }

    public function getPreviousArrow($control) {
        return $this->getArrowFromControlPair($control, false);
    }

    public function getArrowFromControlPair($controlPair, $nextArrow = true, $isDefault = false) {
        if (strpos($controlPair, "|") === false) $controlPair = "auto|auto";
        $split = explode("|", $controlPair);
        $back = $split[0];
        $forward = $split[1];
        $back = $this->fromTerm($back, true);
        $forward = $this->{$forward};
        if ($forward->Term != "auto") {
            $forwardArrow = $forward->Image;
            if ($nextArrow) return $forwardArrow;
        }
        if ($back->Term != "auto") {
            $backArrow = $back->Image;
            if (!$nextArrow) return $backArrow;
        }
        if ($forward->Term == "auto" && $back->Term != "auto") {
            $forward = $this->fromID((int)$back->Implies);
            return $forward->Image;
        }
        if ($back->Term == "auto" && $forward->Term != "auto") {
            $back = $this->fromID((int)$forward->Implies);
            return $back->Image;
        }
        if ($nextArrow) {
            if ($isDefault) return $forward->Image;
            else return "alternate.png";
        } else {
            return $back->Image;
        }
    }
}
?>