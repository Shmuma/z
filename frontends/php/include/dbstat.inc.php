<?php
	global $DB_TYPE;
  global $ROW_SIZE;

  function avg_row_length($table_name, $tries = 1) {
    if ($table_name) {
      $total = 0;

      for ($i=0; $i < $tries; $i++) {
        $row=DBfetch(DBselect("show table status like '" . $table_name . "'"));
        
        $total = $total + $row["Avg_row_length"];
        if ($row["Data_length"] != 0) {
          $total = $total + $row["Avg_row_length"] * $row["Index_length"] / $row["Data_length"];
        }
      }
      
      return round($total / $tries, 0);
    }
  }
  
  if ($DB_TYPE == "MYSQL") {
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_FLOAT] = avg_row_length("history");
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_STR] = avg_row_length("history_str");
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_LOG] = avg_row_length("history_log");
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_UINT64] = avg_row_length("history_uint");
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_TEXT] = avg_row_length("history_text");

    $ROW_SIZE["TRENDS"] = avg_row_length("trends", 10);
  }
  else {
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_FLOAT] = 64;
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_STR] = 160;
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_LOG] = 128;
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_UINT64] = 64;
    $ROW_SIZE["HISTORY"][ITEM_VALUE_TYPE_TEXT] = 160;

    $ROW_SIZE["TRENDS"] = 160;
  }
?>
