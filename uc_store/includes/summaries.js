// $Id: summaries.js,v 1.4 2010-02-01 16:53:50 islandusurper Exp $

/**
 * @file
 *   Adds some helper JS to summaries.
 */

/**
 * Modify the summary overviews to have onclick functionality.
 */
Drupal.behaviors.summaryOnclick = {
  attach: function(context) {
    $('.summary-overview:not(.summaryOnclick-processed)', context).prepend('<img src="' + Drupal.settings.editIconPath + '" class="summary-edit-icon" />');

    $('.summary-overview:not(.summaryOnclick-processed)', context).addClass('summaryOnclick-processed').click(function() {
      window.location = this.id;
    });
  }
}

