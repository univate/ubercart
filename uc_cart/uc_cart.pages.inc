<?php
// $Id: uc_cart.pages.inc,v 1.1.2.2 2008-10-24 21:11:31 islandusurper Exp $

/**
 * @file
 * Cart menu items.
 *
 */

// Displays cart view page with form to adjust cart contents or go to checkout.
function uc_cart_view() {
  // Failsafe so that this function only works when called with no arguments.
  // This prevents the accidental wiping of the cart_order session variable.
  if ((func_num_args() > 0)) {
    return drupal_not_found();
  }

  if (!empty($_SESSION['cart_order'])) {
    unset($_SESSION['cart_order']);
  }

  $items = uc_cart_get_contents();

  if (empty($items)) {
    return theme('uc_empty_cart');
  }

  $panes = uc_cart_cart_pane_list($items);
  foreach ($panes as $pane) {
    if ($pane['enabled']) {
      $output .= $pane['body'];
    }
  }

  if (isset($_SESSION['last_url'])) {
    drupal_set_breadcrumb(array(l(t('Back'), check_plain($_SESSION['last_url']))));
    unset($_SESSION['last_url']);
  }
  else {
    if (($text = variable_get('uc_cart_breadcrumb_text', t('Home'))) !== '') {
      $link = l($text, variable_get('uc_cart_breadcrumb_url', '<front>'));
      drupal_set_breadcrumb(array($link));
    }
  }

  return $output;
}

/**
 * Display the cart checkout page built of checkout panes from enabled modules.
 */
function uc_cart_checkout() {
  global $user;

  $items = uc_cart_get_contents();
  if (count($items) == 0 || !variable_get('uc_checkout_enabled', TRUE)) {
    drupal_goto('cart');
  }

  if (($min = variable_get('uc_minimum_subtotal', 0)) > 0) {
    $subtotal = 0;
    $items = uc_cart_get_contents();
    if (is_array($items) && count($items) > 0) {
      foreach ($items as $item) {
        $data = module_invoke($item->module, 'cart_display', $item);
        if (!empty($data)) {
          $subtotal += $data['#total'];
        }
      }
    }
    if ($subtotal < $min) {
      drupal_set_message(variable_get('uc_minimum_subtotal_text', t('The minimum order subtotal for checkout is !min.', array('!min' => uc_currency_format($min)))), 'error');
      drupal_goto('cart');
    }
  }

  // Send anonymous users to login page when anonymous checkout is disabled.
  if (!$user->uid && !variable_get('uc_checkout_anonymous', TRUE)) {
    drupal_set_message(t('You must login before you can proceed to checkout.'));
    drupal_set_message(t('If you do not have an account, you can <a href="!url">click here</a> to create one.', array('!url' => url('user/register', array('absolute' =>  TRUE)))));
    $_SESSION['checkout-redirect'] = TRUE;
    drupal_goto('user');
  }
  else {
    unset($_SESSION['checkout-redirect']);
  }

  $list = _line_item_list();
  foreach ($list as $line_item) {
    if (function_exists($line_item['callback'])) {
      $line_item['callback']('cart-preview', $items);
    }
  }

  drupal_add_js(drupal_get_path('module', 'uc_cart') .'/uc_cart.js');
  $output = drupal_get_form('uc_cart_checkout_form');

  return $output;
}

function uc_cart_checkout_form() {
  global $user;

  // Cancel an order when a customer clicks the 'Cancel' button.
  if ($_POST['op'] == t('Cancel')) {
    if (intval($_SESSION['cart_order']) > 0) {
      uc_order_comment_save($_SESSION['cart_order'], 0, t('Customer cancelled this order from the checkout form.'));
      unset($_SESSION['cart_order']);
    }
    drupal_goto('cart');
  }

  $order = uc_order_load($_SESSION['cart_order']);

  // Check the referer URI to clear order details and prevent identity theft.
  if (uc_referer_check(array('cart/checkout', 'cart/checkout/review'))) {
    if ($order == FALSE || uc_order_status_data($order->order_status, 'state') != 'in_checkout') {
      unset($_SESSION['cart_order']);
      $order = NULL;
    }
    if (uc_order_status_data($order->order_status, 'state') != 'in_checkout' || ($user->uid > 0 && $user->uid != $order->uid)) {
      $order = NULL;
    }
  }
  else {
    unset($_SESSION['cart_order']);
    $order = NULL;
  }

  $form['panes'] = array('#tree' => TRUE);
  $panes = _checkout_pane_list();

  // If the cart isn't shippable, remove panes with shippable == TRUE.
  if (!uc_cart_is_shippable() && variable_get('uc_cart_delivery_not_shippable', TRUE)) {
    $panes = uc_cart_filter_checkout_panes($panes, array('shippable' => TRUE));
  }

  foreach ($panes as $pane) {
    if (variable_get('uc_pane_'. $pane['id'] .'_enabled', TRUE)) {
      $pane['prev'] = _uc_cart_checkout_prev_pane($panes, $pane['id']);
      $pane['next'] = _uc_cart_checkout_next_pane($panes, $pane['id']);

      if (is_null($pane['collapsed'])) {
        $collapsed = ($pane['prev'] === FALSE || empty($displayed[$pane['prev']])) ? FALSE : TRUE;
      }
      if (isset($_SESSION['expanded_panes'])) {
        if (is_array($_SESSION['expanded_panes']) &&
            in_array($pane['id'], $_SESSION['expanded_panes'])) {
          $collapsed = FALSE;
        }
      }

      $return = $pane['callback']('view', $order, NULL);

      // Add the pane if any display data is returned from the callback.
      if (is_array($return) && (!empty($return['description']) || !empty($return['contents']))) {
        // Create the fieldset for the pane.
        $form['panes'][$pane['id']] = array(
          '#type' => 'fieldset',
          '#title' => $pane['title'],
          '#description' => !empty($return['description']) ? $return['description'] : NULL,
          '#collapsible' => $pane['collapsible'],
          '#collapsed' => variable_get('uc_use_next_buttons', FALSE) ? $collapsed : FALSE,
          '#attributes' => array('id' => $pane['id'] .'-pane'),
          '#theme' => $return['theme'],
        );

        // Add the contents of the fieldset if any were returned.
        if (!empty($return['contents'])) {
          $form['panes'][$pane['id']] = array_merge($form['panes'][$pane['id']], $return['contents']);
        }

        // Add the 'Next' button if necessary.
        if ($return['next-button'] !== FALSE && $pane['next'] !== FALSE &&
            variable_get('uc_use_next_buttons', FALSE) != FALSE) {
          $opt = variable_get('uc_collapse_current_pane', FALSE) ? $pane['id'] : 'false';
          $form['panes'][$pane['id']]['next'] = array(
            '#type' => 'button',
            '#value' => variable_get('uc_checkout_next_button', t('Next')),
            '#weight' => variable_get("uc_pane_{$pane_id}_field_button_weight", 20),
            '#attributes' => array('onclick' => "return uc_cart_next_button_click(this, '". $pane['next'] ."', '". $opt ."');"),
            '#prefix' => '<div class="next-button show-onload">',
            '#suffix' => '</div>',
          );
        }

        // Log that this pane was actually displayed.
        $displayed[$pane['id']] = TRUE;
      }
    }
  }
  unset($_SESSION['expanded_panes']);

  $form['cart_contents'] = array('#type' => 'hidden', '#value' => serialize(uc_cart_get_contents()));
  $form['cancel'] = array('#type' => 'submit', '#submit' => FALSE, '#value' => t('Cancel'));
  $form['continue'] = array('#type' => 'submit', '#value' => variable_get('uc_checkout_review_button', t('Review order')));

  return $form;
}

function theme_uc_cart_checkout_form($form) {
  drupal_add_css(drupal_get_path('module', 'uc_cart') .'/uc_cart.css');

  $output = '<div>'. check_markup(variable_get('uc_checkout_instructions', ''), variable_get('uc_checkout_instructions_format', 3), FALSE) .'</div>';

  if (arg(1) == 'checkout2') {
    foreach (element_children($form['panes']) as $pane_id) {
      $output .= drupal_render($form['panes'][$pane_id]);
    }
  }
  else {
    foreach (element_children($form['panes']) as $pane_id) {
      if (function_exists(($func = _checkout_pane_data($pane_id, 'callback')))) {
        $result = $func('theme', $form['panes'][$pane_id], NULL);
        if (!empty($result)) {
          $output .= $result;
          $form['panes'][$pane_id] = array();
        }
        else {
          $output .= drupal_render($form['panes'][$pane_id]);
        }
      }
      else {
        $output .= drupal_render($form['panes'][$pane_id]);
      }
    }
  }

  $output .= '<div id="checkout-form-bottom">'. drupal_render($form) .'</div>';

  return $output;
}

function uc_cart_checkout_form_validate($form, &$form_state) {
  global $user;

  if (empty($_SESSION['cart_order'])) {
    $order = uc_order_new($user->uid);
    $_SESSION['cart_order'] = $order->order_id;
  }
  else {
    $order = new stdClass();
    $order->uid = $user->uid;
    $order->order_id = $_SESSION['cart_order'];
    $order->order_status = uc_order_state_default('in_checkout');
  }

  $order->products = unserialize($form_state['values']['cart_contents']);

  $order->order_total = uc_order_get_total($order, TRUE);

  // Validate/process the cart panes.  A FALSE value results in failed checkout.
  $_SESSION['checkout_valid'] = TRUE;
  foreach (element_children($form_state['values']['panes']) as $pane_id) {
    $func = _checkout_pane_data($pane_id, 'callback');
    $isvalid = $func('process', $order, $form_state['values']['panes'][$pane_id]);
    if ($isvalid === FALSE) {
      $_SESSION['expanded_panes'][] = $pane_id;
      $_SESSION['checkout_valid'] = FALSE;
    }
  }

  $order->line_items = uc_order_load_line_items($order->order_id, TRUE);

  uc_order_save($order);
}

function uc_cart_checkout_form_submit($form, &$form_state) {
  if ($_SESSION['checkout_valid'] === FALSE) {
    $url = 'cart/checkout';
  }
  else {
    $url = 'cart/checkout/review';
    $_SESSION['do_review'] = TRUE;
  }

  unset($_SESSION['checkout_valid']);

  $form_state['redirect'] = $url;
}

/**
 * Allow a customer to review their order before finally submitting it.
 */
function uc_cart_checkout_review() {
  drupal_add_js(drupal_get_path('module', 'uc_cart') .'/uc_cart.js');
  $form = drupal_get_form('uc_cart_checkout_review_form');

  if ($_SESSION['do_review'] !== TRUE && !uc_referer_check('cart/checkout')) {
    drupal_goto('cart/checkout');
  }
  unset($_SESSION['do_review']);

  $order = uc_order_load($_SESSION['cart_order']);

  if ($order === FALSE || uc_order_status_data($order->order_status, 'state') != 'in_checkout') {
    unset($_SESSION['cart_order']);
    drupal_goto('cart/checkout');
  }

  $panes = _checkout_pane_list();

  // If the cart isn't shippable, bypass panes with shippable == TRUE.
  if (!uc_cart_is_shippable() && variable_get('uc_cart_delivery_not_shippable', TRUE)) {
    $panes = uc_cart_filter_checkout_panes($panes, array('shippable' => TRUE));
  }

  foreach ($panes as $pane) {
    if (variable_get('uc_pane_'. $pane['id'] .'_enabled', TRUE)) {
      $func = $pane['callback'];
      if (function_exists($func)) {
        $return = $func('review', $order, NULL);
        if (!is_null($return)) {
          $data[$pane['title']] = $return;
        }
      }
    }
  }

  $output = theme('uc_cart_checkout_review', $data, $form);

  return $output;
}

/**
 * Theme the checkout review order page.
 *
 * @param $panes
 *   An associative array for each checkout pane that has information to add to
 *   the review page.  The key is the pane's title and the value is either the
 *   data returned for that pane or an array of returned data.
 * @param $form
 *   The HTML version of the form that by default includes the 'Back' and
 *   'Submit order' buttons at the bottom of the review page.
 * @return
 *   A string of HTML for the page contents.
 * @ingroup themeable
 */
function theme_uc_cart_checkout_review($panes, $form) {
  drupal_add_css(drupal_get_path('module', 'uc_cart') .'/uc_cart.css');

  $output = '<p>' . check_markup(variable_get('uc_checkout_review_instructions', uc_get_message('review_instructions')), variable_get('uc_checkout_review_instructions_format', 3), FALSE)
           . '</p><table class="order-review-table">';

  foreach ($panes as $title => $data) {
    $output .= '<tr class="pane-title-row"><td colspan="2">'. $title
              .'</td></tr>';
    if (is_array($data)) {
      foreach ($data as $row) {
        if (is_array($row)) {
          if (isset($row['border'])) {
            $border = ' class="row-border-'. $row['border'] .'"';
          }
          else {
            $border = '';
          }
          $output .= '<tr valign="top"'. $border .'><td class="title-col" '
                    .'nowrap>'. $row['title'] .':</td><td class="data-col">'
                   . $row['data'] .'</td></tr>';
        }
        else {
          $output .= '<tr valign="top"><td colspan="2">'. $row .'</td></tr>';
        }
      }
    }
    else {
      $output .= '<tr valign="top"><td colspan="2">'. $data .'</td></tr>';
    }
  }

  $output .= '<tr class="review-button-row"><td colspan="2">'. $form
            .'</td></tr></table>';

  return $output;
}

function uc_cart_checkout_review_form() {
  // Set the session variable to pass the redirect check on the pageload.
  if ($_POST['op'] == t('Back')) {
    $_SESSION['do_review'] = TRUE;
  }

  $form['back'] = array(
    '#type' => 'submit',
    '#submit' => array('uc_cart_checkout_review_form_back'),
    '#value' => t('Back'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => variable_get('uc_checkout_submit_button', t('Submit order')),
  );

  return $form;
}

function uc_cart_checkout_review_form_back($form, &$form_state) {
  cache_clear_all();
  unset($_SESSION['do_review']);
  $form_state['redirect'] = 'cart/checkout';
}

function uc_cart_checkout_review_form_submit($form, &$form_state) {
  // Invoke hook_order($op = 'submit') to test to make sure the order can
  // be completed... used for auto payment in uc_credit.module.
  $order = uc_order_load($_SESSION['cart_order']);
  $pass = module_invoke_all('order', 'submit', $order, NULL);

  foreach ($pass as $result) {
    $msg_type = 'status';
    if ($result['pass'] === FALSE) {
      $error = TRUE;
      $msg_type = 'error';
    }
    if (!empty($result['message'])) {
      drupal_set_message($result['message'], $msg_type);
    }
  }

  if ($error === TRUE) {
    $_SESSION['do_review'] = TRUE;
    $form_state['redirect'] = 'cart/checkout/review';
  }
  else {
    $_SESSION['do_complete'] = TRUE;
    $form_state['redirect'] = 'cart/checkout/complete';
  }
}

function uc_cart_checkout_complete() {
  if (!$_SESSION['do_complete']) {
    drupal_goto('cart');
  }

  $order = uc_order_load(intval($_SESSION['cart_order']));

  if (empty($order)) {
    // Display messages to customers and the administrator if the order was lost.
    drupal_set_message(t("We're sorry.  An error occurred while processing your order that prevents us from completing it at this time. Please contact us and we will resolve the issue as soon as possible."), 'error');
    watchdog('uc_cart', 'An empty order made it to checkout! Cart order ID: @cart_order', array('@cart_order' => $_SESSION['cart_order']), WATCHDOG_ERROR);
    drupal_goto('cart');
  }

  $output = uc_cart_complete_sale($order, variable_get('uc_new_customer_login', FALSE));

  // Add a comment to let sales team know this came in through the site.
  uc_order_comment_save($order->order_id, 0, t('Order created through website.'), 'admin');

  $page = variable_get('uc_cart_checkout_complete_page', '');
  if (!empty($page)) {
    drupal_goto(variable_get('uc_cart_checkout_complete_page', ''));
  }

  return $output;
}
