=== Trailmeme for WordPress ===
Contributors: jbsil
Tags: wordtrails, trailmeme, trail, trails, xerox
Requires at least: 2.5.2
Tested up to: 3.0
Stable tag: trunk

Trailmeme for WordPress (formerly WordTrails) allows for the creation of multi-branched trails of navigation through WordPress Blogs

== Description ==

Wouldn't you like to offer your readers a "best of my blog" feature that is easy for you to maintain? Or maybe a series of posts that add up to a novel? Or a tutorial on your area of expertise?

You can do all that and more with Trailmeme for WordPress, one of the products in the Trailmeme ecosystem. Trailmeme for WordPress allows you to blaze trails on your WordPress blog that users can easily navigate with a sidebar widget and a Flash map. The benefits are:

*	[Drive traffic and increase session length](http://blog.trailmeme.com/2009/12/drive-traffic-using-trailmeme-for-wordpress/)
*	[Allow your readers to create instant PDFs](http://blog.trailmeme.com/2009/12/print-enabling-your-blog-with-trailmeme/)
*	[Get real-time analytics that tell you which trails are being followed](http://blog.trailmeme.com/2009/12/understanding-trailmeme-analytics/)

There are many other benefits. You may want to subscribe to the [Trailmeme Blog](http://blog.trailmeme.com) for hints, tips, updates and coaching. You will also find more extensive tutorials there (organized as trails of course!)

== Installation ==

1. Download, install and activate the plugin from your admin panel directly from WordPress.org as with any other plugin, or drop it into the plugins folder for a manual install.

1. Follow the admin instructions to create a page that the plugin can "hijack" as its home. The page is named "Trails" by default. Then add the widget to your sidebar (you need a widget-enabled theme).

New to Trails? [Learn more](http://blog.trailmeme.com/new-to-trails/), or jump right in with the [Quick start guide](http://blog.trailmeme.com/2009/09/wordtrails-quickstart/)

== Changelog ==

= 0.9.8.1 =
* Bugfix: Getting the excerpt of the Trail Index or Trail Map pages caused a massive failure
    * This fixes Thesis 1.7 Compatibility (new meta description tag uses excerpt)
* Bugfix: Revert to Saved for never-saved trails caused a full delete
* Bugfix: PHP Warning when displaying detailed analytics for Trails that have never been followed
* Some minor verbiage changes working towards better usability
* Added New, Updated and Editing flags to Trails Index

= 0.9.8 =
* Rewritten Analytics calculations
* 'Intro text' for Trail Index page can be edited from Pages > Edit > Trails (or your page name)
* Widget title customizable
* Graceful plugin activation failure on PHP4 systems

= 0.9.7.2 =
* Trail slug database error fix
* dbDelta database upgrade fix
    * 'Default' field not supported by dbDelta
    * Renamed field to Def, changed references in classes
    * Added alter table script to upgrade script
        * Check for old Default field
        * Rename to Def before running dbDelta
* Trail Index is paginated (10)
* Unset two DB Results array while crunching data to (help) prevent script death

= 0.9.7.1 =
* Renamed 0.9.7a to 0.9.7.1 since 0.9.7a refused to register as an automatic upgrade. Hopefully .1 works.

= 0.9.7a =
* Minor fix for Trail Index flag status
* Logical fix for PDF image conversion (reduces error reporting)
* Fixed: WAY too much forced error reporting was left in
* Fixed: Possible mySQL error where LAST_INSERT_ID does not function. Added query to find correct ID in multiple locations.

= 0.9.7 =
* Fixes for Pretty URLs
* Fixes for Analytics with Pretty URLs enabled
* One-click save (auto-save from Trail Viewer)
* Language consistency

= 0.9.5 =
* _Pretty URLs!_ Trail Map pages, walk pages, printing, etc. all have pretty URLs now! Better for search engines, and better for your eyes.

= 0.9.4 =
* Added extensive user-input sanitization for safety
* Added "trailmeme-enabled" metadata to Trails Index listing
* Added non-widget-enabled theme notice
* Added option to disable TM4WP notices individually - reset on settings page
* Added hover bubbles to the Trail Map for meta data

= 0.9.3 =
* Changed Trail Index click behaviors to match desired behaviors on Trailmeme.com
* Rounded Browse Time on Trail Index > 60 to nearest hour (e.g. 190 mins -> About 3 hours)
* Removed sort by Created timestamp before print - fixes (bypasses) possible Apache thread crash during sorting
    * _N.B. This means pages on a trail that are NOT connected to other pages will show up in an unreliable order in PDF generation_
    * This should slightly improve PDF load times
    * TODO: Cache PDFs
* More code cleanup, mainly for sorting functions

= 0.9.2a =
* Fixed 2 more division by zero PHP Warnings that I missed last time
>(cannot calculate average read depth for a trail when only looking up statistics for a single session)

= 0.9.2 =
* Some Code Cleanup
* Fixed PHP Warnings from Analytics crunching

= 0.9.1 =
* Changed name to Trailmeme for WordPress (used to be WordTrails)
* Changed Trialmeme registration to use https
* XHTML 1.0 Strict Compliance (or at least significantly closer to it)
* Fixed foreach error for themes without sidebars
* Fixed URL creation error for admin trail edit interface - now uses admin_url (with backward compat.)
* Fixed unable to delete meta data bug (overwrite worked, delete did not)
* Reworked all of the Analytics calculations and display

= 0.9.0c =
* Fixed double folder issue with SVN, causing automatic install/upgrade to fail

= 0.9.0b =
* Added plain_quotes parser to XML generation methods for cleaner transmission
* Added sync is still beta notice

= 0.9.0a =
* Added Stable tag to this readme for parser
* Fixed a bug where the site hash was never generated

= 0.9.0 =
* Copyright/Author information updated
* Notes about TrailViewer source added to this file and wordtrails.php
* Public beta release

= 0.8.9 =
* Basic feature set finished.
* All known bugs squashed.
* Ready for testing on public blogs: [http://ribbonfarm.com] & [http://chaosprg.com]

== Upgrade Notice ==

= 0.9.7.2 =
* Automatic DB Upgrade fix
* Trail Index is paginated (10 per page)

= 0.9.7.1 =
* Bug fixes! Yay!

= 0.9.7 =
* One-click Saves!
* Pretty URLs!

= 0.9.4 =
* Hover bubbles in Trail Map!
* Option to disable TM4WP warnings

= 0.9.3 =
* Possible server crash fix
* Better usability on Trail Index

= 0.9.2a =
* Fixed more PHP Warnings

= 0.9.2 =
* Fixed PHP Warnings

== GPL Source Note ==

For compactness, the TrailViewer source files (.fla and .as) are not included in this package. They can be downloaded from [our blog](http://blog.trailmeme.com/trailviewer-source/).

== Upcoming Features! ==

* Background processing of Analytics so you don't grow old waiting for the page to load every single time!
* Trail Map snap-to-grid during editing - line those nodes up!
* Support for non-widget enabled themes - php code snippet to add to your theme files, if you do not want to use the widget
* Internationalization