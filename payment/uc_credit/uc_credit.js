// $Id: uc_credit.js,v 1.6 2010-02-10 19:21:50 islandusurper Exp $
(function($) {

$(document).ready(
  function () {
    $('#cc_details_title').show(0);
    $('#cc_details').hide(0);
  }
);

/**
 * Toggle credit card details on the order view screen.
 */
function toggle_card_details() {
  $('#cc_details').toggle();
  $('#cc_details_title').toggle();
}

})(jQuery);
