<?php
// $Id: uc_payment_payment_method.inc,v 1.4 2008-07-10 12:40:57 islandusurper Exp $

/**
 * Handle the generic payment method "Other."
 */
function uc_payment_method_other($op, &$arg1) {
  switch ($op) {
    case 'order-view':
    case 'customer-view':
      $result = db_query("SELECT description FROM {uc_payment_other} WHERE order_id = %d", $arg1->order_id);
      if ($row = db_fetch_object($result)) {
        $output = t('Type: @description', array('@description' => $row->description));
      }
      else {
        $output = t('Type: Unknown');
      }
      return $output;

    case 'order-details':
      $details = drupal_get_form('uc_payment_method_other_form', $arg1);
      return uc_strip_form($details);

    case 'edit-process':
      $changes['payment_details']['pm_other_description'] = check_plain($_POST['pm_other_description']);
      return $changes;

    case 'order-load':
      $result = db_query("SELECT description FROM {uc_payment_other} WHERE order_id = %d", $arg1->order_id);
      if ($row = db_fetch_object($result)) {
        $arg1->payment_details['description'] = $row->description;
      }
      break;

    case 'order-save':
      db_query("DELETE FROM {uc_payment_other} WHERE order_id = %d", $arg1->order_id);
      if (strlen($arg1->payment_details['pm_other_description']) > 0) {
        db_query("INSERT INTO {uc_payment_other} (order_id, description) VALUES "
                ."(%d, '%s')", $arg1->order_id,
                $arg1->payment_details['pm_other_description']);
      }
      break;
  }
}

function uc_payment_method_other_form($order) {
  $form['pm_other_description'] = array(
    '#type' => 'textfield',
    '#size' => 32,
    '#maxlength' => 64,
    '#default_value' => $order->payment_details['description'],
  );

  return $form;
}

function theme_uc_payment_method_other_form($form) {
  $output = '<table class="order-edit-table"><tr><td class="oet-label">'
          . t('Description:') .'</td><td>'
          . drupal_render($form['pm_other_description'])
          .'</td></tr></table>';
  return $output;
}

/**
 * Handle the Cash on Delivery payment method.
 */
function uc_payment_method_cod($op, &$arg1) {
  switch ($op) {
    case 'cart-details':
      $details = variable_get('uc_cod_policy', t('Full payment is expected upon delivery or prior to pick-up.'));
      if (($max = variable_get('uc_cod_max_order', 0)) > 0 && is_numeric($max)) {
        $details .= '<p>'. t('Orders totalling more than !number are <b>not eligible</b> for COD.', array('!number' => uc_currency_format($max))) .'</p>';
      }
      if (variable_get('uc_cod_delivery_date', FALSE)) {
        $details .= uc_strip_form(drupal_get_form('uc_payment_method_cod_form', $arg1));
      }
      return $details;

    case 'cart-process':
      if (variable_get('uc_cod_delivery_date', FALSE)) {
        $arg1->payment_details['delivery_month'] = intval($_POST['cod_delivery_month']);
        $arg1->payment_details['delivery_day'] = intval($_POST['cod_delivery_day']);
        $arg1->payment_details['delivery_year'] = intval($_POST['cod_delivery_year']);
      }
      return TRUE;

    case 'cart-review':
      if (variable_get('uc_cod_delivery_date', FALSE)) {
        $date = uc_date_format($arg1->payment_details['delivery_month'],
                               $arg1->payment_details['delivery_day'],
                               $arg1->payment_details['delivery_year']);
        $review[] = array('title' => t('Delivery Date'), 'data' => $date);
      }
      return $review;

    case 'order-view':
      if (variable_get('uc_cod_delivery_date', FALSE)) {
        $output = t('Desired delivery date:') .'<br />'
                . uc_date_format($arg1->payment_details['delivery_month'],
                                 $arg1->payment_details['delivery_day'],
                                 $arg1->payment_details['delivery_year']);
      }
      return $output;

    case 'order-details':
      if (variable_get('uc_cod_delivery_date', FALSE)) {
        $details .= uc_strip_form(drupal_get_form('uc_payment_method_cod_form', $arg1));
      }
      return $details;

    case 'order-load':
      $result = db_query("SELECT * FROM {uc_payment_cod} WHERE order_id = %d", $arg1->order_id);
      if ($row = db_fetch_object($result)) {
        $arg1->payment_details['delivery_month'] = $row->delivery_month;
        $arg1->payment_details['delivery_day'] = $row->delivery_day;
        $arg1->payment_details['delivery_year'] = $row->delivery_year;
      }
      break;

    case 'order-submit':
      if ($arg1->payment_method == 'cod' &&
          ($max = variable_get('uc_cod_max_order', 0)) > 0 &&
          is_numeric($max) &&
          $arg1->order_total > $max) {
          $result[] = array(
            'pass' => FALSE,
            'message' => t('Your final order total exceeds the maximum for COD payment.  Please go back and select a different method of payment.')
          );
          $_SESSION['expanded_panes'][] = 'payment';
          return $result;
      }
    case 'order-save':
      db_query("DELETE FROM {uc_payment_cod} WHERE order_id = %d", $arg1->order_id);
      db_query("INSERT INTO {uc_payment_cod} VALUES (%d, %d, %d, %d)",
               $arg1->order_id, $arg1->payment_details['delivery_month'],
               $arg1->payment_details['delivery_day'], $arg1->payment_details['delivery_year']);
      break;

    case 'order-delete':
      db_query("DELETE FROM {uc_payment_cod} WHERE order_id = %d", $arg1->order_id);
      break;

    case 'settings':
      $form['uc_cod_max_order'] = array(
        '#type' => 'textfield',
        '#title' => t('Maximum order total eligible for COD'),
        '#default_value' => variable_get('uc_cod_max_order', 0),
        '#description' => t('Set to 0 for no maximum order limit.'),
      );
      $form['uc_cod_delivery_date'] = array(
        '#type' => 'checkbox',
        '#title' => t('Let customers enter a desired delivery date.'),
        '#default_value' => variable_get('uc_cod_delivery_date', FALSE),
      );
      return $form;
  }
}

function uc_payment_method_cod_form($order) {
  $form['table1'] = array('#value' => '<style>.cod-table div, .cod-table { display: inline; }</style><table class="cod-table"><tr><td class="form-item" colspan="3"><label>'. t('Enter a desired delivery date:') .'</label></td></tr><tr><td>');
  $month = (!empty($order->payment_details['delivery_month'])) ? $order->payment_details['delivery_month'] : format_date(time(), 'custom', 'n');
  $form['cod_delivery_month'] = uc_select_month(NULL, $month);
  $form['table2'] = array('#value' => '</td><td>');
  $day = (!empty($order->payment_details['delivery_day'])) ? $order->payment_details['delivery_day'] : format_date(time(), 'custom', 'j');
  $form['cod_delivery_day'] = uc_select_day(NULL, $day);
  $form['table3'] = array('#value' => '</td><td>');
  $year = (!empty($order->payment_details['delivery_year'])) ? $order->payment_details['delivery_year'] : format_date(time(), 'custom', 'Y');
  $form['cod_delivery_year'] = uc_select_year(NULL, $year, format_date(time(), 'custom', 'Y'), format_date(time(), 'custom', 'Y') + 1);
  $form['table4'] = array('#value' => '</td></tr></table>');

  return $form;
}

/**
 * Handle the Check payment method.
 */
function uc_payment_method_check($op, &$arg1) {
  switch ($op) {
    case 'cart-details':
      if (!variable_get('uc_check_mailing_street1', FALSE)) {
        $details = t('Checks should be made out to:') .'<p>'
                 . uc_address_format(variable_get('uc_store_name', ''),
                                     NULL,
                                     variable_get('uc_store_company', ''),
                                     variable_get('uc_store_street1', ''),
                                     variable_get('uc_store_street2', ''),
                                     variable_get('uc_store_city', ''),
                                     variable_get('uc_store_zone', ''),
                                     variable_get('uc_store_postal_code', ''),
                                     variable_get('uc_store_country', 840))
                  .'</p><p>'. variable_get('uc_check_policy', '') .'</p>';
      }
      else {
        $details = t('Checks should be made out to:') .'<p>'
                 . uc_address_format(variable_get('uc_check_mailing_name', ''),
                                     NULL,
                                     variable_get('uc_check_mailing_company', ''),
                                     variable_get('uc_check_mailing_street1', ''),
                                     variable_get('uc_check_mailing_street2', ''),
                                     variable_get('uc_check_mailing_city', ''),
                                     variable_get('uc_check_mailing_zone', ''),
                                     variable_get('uc_check_mailing_postal_code', ''),
                                     variable_get('uc_check_mailing_country', 840))
                  .'</p><p>'. variable_get('uc_check_policy', '') .'</p>';
      }
      return $details;

    case 'cart-review':
      if (!variable_get('uc_check_mailing_street1', FALSE)) {
        $review[] = array('title' => t('Mail to'), 'data' =>
                   uc_address_format(variable_get('uc_store_name', ''),
                                     NULL,
                                     variable_get('uc_store_company', ''),
                                     variable_get('uc_store_street1', ''),
                                     variable_get('uc_store_street2', ''),
                                     variable_get('uc_store_city', ''),
                                     variable_get('uc_store_zone', ''),
                                     variable_get('uc_store_postal_code', ''),
                                     variable_get('uc_store_country', 840)));
      }
      else {
        $review[] = array('title' => t('Mail to'), 'data' =>
                   uc_address_format(variable_get('uc_check_mailing_name', ''),
                                     NULL,
                                     variable_get('uc_check_mailing_company', ''),
                                     variable_get('uc_check_mailing_street1', ''),
                                     variable_get('uc_check_mailing_street2', ''),
                                     variable_get('uc_check_mailing_city', ''),
                                     variable_get('uc_check_mailing_zone', ''),
                                     variable_get('uc_check_mailing_postal_code', ''),
                                     variable_get('uc_check_mailing_country', 840)));
      }
      return $review;

    case 'order-view':
      if (!variable_get('uc_payment_tracking', TRUE)) {
        return '';
      }
      $result = db_query("SELECT clear_date FROM {uc_payment_check} WHERE "
                        ."order_id = %d ", $arg1->order_id);
      if ($check = db_fetch_object($result)) {
        $output = t('Clear Date:') .' '. format_date($check->clear_date, 'custom', variable_get('uc_date_format_default', 'm/d/Y'));
      }
      else {
        $output = l(t('Receive Check'), 'admin/store/orders/'. $arg1->order_id .'/receive_check');
      }
      $output .= '<br />';
      return $output;

    case 'customer-view':
      if (!variable_get('uc_payment_tracking', TRUE)) {
        return '';
      }
      $result = db_query("SELECT clear_date FROM {uc_payment_check} WHERE "
                        ."order_id = %d ", $arg1->order_id);
      if ($check = db_fetch_object($result)) {
        $output = t('Check received') .'<br />'
                . t('Expected clear date:') .'<br />'. format_date($check->clear_date, 'custom', variable_get('uc_date_format_default', 'm/d/Y'));
      }
      return $output;

    case 'settings':
      $form['check_address_info'] = array(
        '#value' => '<div>'. t('Set the mailing address to display to customers who choose this payment method during checkout.') .'</div>',
      );
      $form['uc_check_mailing_company'] = uc_textfield(uc_get_field_name('company'), variable_get('uc_check_mailing_company', ''), FALSE, NULL, 128);
      $form['uc_check_mailing_name'] = uc_textfield(t('Contact'), variable_get('uc_check_mailing_name', ''), FALSE, t('Direct checks to a person or department.'), 128);
      $form['uc_check_mailing_street1'] = uc_textfield(uc_get_field_name('street1'), variable_get('uc_check_mailing_street1', ''), FALSE, NULL, 128);
      $form['uc_check_mailing_street2'] = uc_textfield(uc_get_field_name('street2'), variable_get('uc_check_mailing_street2', ''), FALSE, NULL, 128);
      $form['uc_check_mailing_city'] = uc_textfield(uc_get_field_name('city'), variable_get('uc_check_mailing_city', ''), FALSE);
      $form['uc_check_mailing_zone'] = uc_zone_select(uc_get_field_name('zone'), variable_get('uc_check_mailing_zone', ''), FALSE);
      $form['uc_check_mailing_postal_code'] = uc_textfield(uc_get_field_name('postal_code'), variable_get('uc_check_mailing_postal_code', ''), FALSE, NULL, 10, 10);
      $form['uc_check_mailing_country'] = uc_country_select(uc_get_field_name('country'), variable_get('uc_check_mailing_country', 840));
      $form['uc_check_policy'] = array(
        '#type' => 'textarea',
        '#title' => t('Check payment policy'),
        '#description' => t('Instructions for customers on the checkout page.'),
        '#default_value' => variable_get('uc_check_policy', t('Personal and business checks will be held for up to 10 business days to ensure payment clears before an order is shipped.')),
        '#rows' => 3,
      );
      return $form;
  }
}

/**
 * Receive a check for an order and put in a clear date.
 */
function uc_payment_receive_check_form($order_id) {
  $order = uc_order_load($order_id);

  if ($order === FALSE) {
    drupal_set_message(t('Order !order_id does not exist.', array('!order_id' => $order_id)));
    drupal_goto('admin/store/orders');
  }
  $balance = uc_payment_balance($order);
  $form['balance'] = array('#value' => uc_currency_format($balance));
  $form['order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['check_exists'] = array(
    '#type' => 'checkbox',
    '#title' => t('Check has already been received.'),
    '#attributes' => array('onclick' => 'receive_check_toggle(this.checked);'),
  );
  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#description' => t('Do not include currency sign.'),
    '#default_value' => uc_currency_format($balance, FALSE, FALSE),
    '#size' => 10,
  );
  $form['comment'] = array(
    '#type' => 'textfield',
    '#title' => t('Comment'),
    '#description' => t('Any notes about the check, like type or check number.'),
    '#size' => 64,
    '#maxlength' => 256,
  );
  $form['clear_month'] = uc_select_month(NULL, format_date(time(), 'custom', 'n'));
  $form['clear_day'] = uc_select_day(NULL, format_date(time(), 'custom', 'j'));
  $form['clear_year'] = uc_select_year(NULL, format_date(time(), 'custom', 'Y'), format_date(time(), 'custom', 'Y'), format_date(time(), 'custom', 'Y') + 1);
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Receive check'),
  );

  return $form;
}

function theme_uc_payment_receive_check_form($form) {
  uc_add_js(drupal_get_path('module', 'uc_payment') .'/uc_payment.js');

  $output = '<p>'. t('Use the form to enter the check into the payments system and set the expected clear date.') .'</p>';
  $output .= '<p><strong>'. t('Order balance:') .'</strong> '
           . drupal_render($form['balance']) .'</p>';

  $output .= drupal_render($form['check_exists'])
           . drupal_render($form['amount'])
           . drupal_render($form['comment']);

  $output .= '<table><tr><td colspan="3"><strong>'. t('Expected clear date:')
            .'</strong></td></tr><tr><td>'. drupal_render($form['clear_month'])
            .'</td><td>'. drupal_render($form['clear_day']) . '</td><td>'
           . drupal_render($form['clear_year']) .'</td></tr></table>';

  $output .= drupal_render($form);

  return $output;
}

function uc_payment_receive_check_form_validate($form_id, $form_values) {
  if (!$form_values['check_exists'] && !is_numeric($form_values['amount'])) {
    form_set_error('amount', t('The amount must be a number.'));
  }
}

function uc_payment_receive_check_form_submit($form_id, $form_values) {
  global $user;

  uc_payment_enter($form_values['order_id'], _payment_method_data('check', 'name'),
                   $form_values['amount'], $user->uid, '', $form_values['comment']);

  db_query("INSERT INTO {uc_payment_check} (check_id, order_id, clear_date) "
          ."VALUES (%d, %d, %d)", db_next_id('{uc_payment_check}_check_id'),
           $form_values['order_id'], mktime(12, 0, 0, $form_values['clear_month'], $form_values['clear_day'], $form_values['clear_year']));

  drupal_set_message(t('Check received, expected clear date of !date.',
                     array('!date' => $form_values['clear_month'] .'/'
                                    . $form_values['clear_day'] .'/'
                                    . $form_values['clear_year'])));

  drupal_goto('admin/store/orders/'. $form_values['order_id']);
}
