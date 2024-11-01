<?php
/*
Plugin Name: Trailmeme for WordPress
Plugin URI: http://trailmeme.com/tools/WordPress
Description: Trailmeme for WordPress (formerly WordTrails) allows for the creation of multi-branched trails of navigation through WordPress Blogs
Author: Jesse Silverstein (Xerox Corporation)
Version: 0.9.8.1
Author URI: http://blog.trailmeme.com/
*/

/*  Copyright 2008-2010  Xerox Corporation  (email : jesse.silverstein@xerox.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

/**
 * Note: For compactness, the TrailViewer source files (.fla and .as) are not
 * included in this package. They can be downloaded from
 * <http://open.xerox.com/xeroxproject/xerox-trails>
 */

/**
 * This is the initial file for the Trailmeme for WordPress plugin.
 *
 * This file contains all of the initialization functions for Trailmeme for WordPress. It
 * adds all of the action and filter hooks necesary.
 *
 * @package Trailmeme for WordPress
 * @since 0.0.1
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008-2009 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */


if ($_GET['action'] == 'error_scrape') {
    $fail = get_option("tm4wp_install_fail");
    delete_option("tm4wp_install_fail");
    if ($fail == "php5") {
	die("Sorry, Trailmeme for WordPress requires PHP 5.0 or higher. Please upgrade to or enable PHP5.");
    }
}

// {{{ wp_wt_install()

/**
 * Registers the Trailmeme for WordPress plugin with WordPress
 *
 * @global $wpdb WordPress database management class
 */
function wp_wt_install() {
    if (version_compare(phpversion(), "5.0", "<")) {
	update_option("tm4wp_install_fail", "php5");
	$current = get_option('active_plugins');
	$plugin = plugin_basename(__FILE__);
	$key = array_search( $plugin, (array) $current );
	if ( false !== $key ) {
	    array_splice( $current, $key, 1 );
	    update_option('active_plugins', $current);
	}
        trigger_error('', E_USER_ERROR);
	return false;
    }
    require_once "includes/activate.inc";
}
// }}}
register_activation_hook(__FILE__,'wp_wt_install');

function wp_wt_deactivate() {
    if (version_compare(phpversion(), "5.0", ">=")) {
	require_once "includes/deactivate.inc";
    }
}
register_deactivation_hook(__FILE__, 'wp_wt_deactivate');

if (version_compare(phpversion(), "5.0", ">=")) {
    require_once "includes/hooks.inc";
} //version check
?>