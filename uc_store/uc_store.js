// $Id: uc_store.js,v 1.13 2010-02-10 19:21:50 islandusurper Exp $
(function($) {

/**
 * @file
 * Add click events to the orders table and administration dashboard.
 */

/**
 * Add the "show links" click behavior on the store admin display.
 */
Drupal.behaviors.ucStoreMoreLinks = {
  attach: function(context, settings) {
    $('.uc-store-admin-panel:not(.ucStoreMoreLinks-processed)', context).addClass('ucStoreMoreLinks-processed').each(
      function() {
        var panel_id = this.id.substring(6);
        $('#show-links-' + panel_id).click(
          function() {
            var panel_id = this.id.substring(11);
            $('#panel-' + panel_id + ' .panel-links').toggle();
            if ($('#panel-' + panel_id + ' .panel-show-link').html() == '<a>' + settings.ucTextShow + '</a>') {
              $('#panel-' + panel_id + ' .panel-show-link').html('<a>' + settings.ucTextHide + '</a>');
            }
            else {
              $('#panel-' + panel_id + ' .panel-show-link').html('<a>' + settings.ucTextShow + '</a>');
            }
          }
        );
      }
    );
  }
}

/**
 * Add the double click behavior to the order table rows
 */
Drupal.behaviors.ucCustomerOrder = {
  attach: function(context, settings) {
    $('.uc-customer-table tr.odd, .uc-customer-table tr.even:not(.ucCustomerOrder-processed)', context).addClass('ucCustomerOrder-processed').each(
      function() {
        $(this).dblclick(
          function() {
            window.location = settings.basePath + '?q=admin/store/customers/orders/' + this.id.substring(9);
          }
        );
      }
    );
  }
}

/**
 * Add the double click to the customer orders table rows.
 */
Drupal.behaviors.ucCustomerOrders = {
  attach: function(context, settings) {
    $('.uc-cust-orders-table tr.odd, .uc-cust-orders-table tr.even:not(.ucCustomerOrders-processed)', context).addClass('ucCustomerOrders-processed').each(
      function() {
        $(this).dblclick(
          function() {
            window.location = settings.basePath + '?q=admin/store/orders/' + this.id.substring(6);
          }
        );
      }
    );
  }
}

})(jQuery);
