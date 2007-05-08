// $Id: uc_country_select.js,v 1.1 2007-05-08 21:15:31 rszrama Exp $

$(document).ready(
  function() {
    $('select[@id$=-country]').change(
      function() {
        uc_update_zone_select(this.id, 0);
      }
    );
  }
);

function uc_update_zone_select(country_select, default_zone) {
  var zone_select = country_select.substr(0, country_select.length - 8) + '-zone';

  var options = { 'country_id' : $('#' + country_select).val() };

  $.post(base_path + '?q=uc_js_util/zone_select', options,
         function (contents) {
           $('#' + zone_select).empty().append(contents).val(default_zone);
         }
  );
}
