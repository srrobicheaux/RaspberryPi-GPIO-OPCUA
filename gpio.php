<?php

function AddSpan(int $Id=85, int $NameSpace=0, bool $UseValue,  $val){
  unset($results);
  unset($lines);
  unset($output2);

  if ($UseValue){
    $value = (strcmp($val, "true") ==0) ? " -value true" : "-value false";
}
  else {
    $value="";
  };


  $cmd = escapeshellcmd("/home/pi/open62541/build/toggle -node ".$Id." -namespace ". $NameSpace ." ". $value ." opc.tcp://192.168.2.190:4840");
  exec($cmd, $lines);
  $output2 = print_r($lines, true);
  $results = explode("UA_Response", utf8_encode(trim($output2)))[1];
  $ND = json_decode($results, true);
  echo "<li id='".$Id."' namespace='".$NameSpace."' >";
  echo "<span class='box'";

  foreach($ND as $key => $value){
    echo " ".$key."='".$value."'";
    if (strcmp($key, "Current") == 0){
      echo " type='". gettype( $ND['Current'] ) . "'";
    };
  };
  echo ">".$ND["DisplayName"]."</span></li>";  
};

$NodeId = $_REQUEST["nodeid"];
$NameSpace = $_REQUEST["ns"];
$value = $_REQUEST["value"];

if(isset($value)){
  AddSpan( $NodeId, $NameSpace, true, $value);
}
else {

  AddSpan( $NodeId, $NameSpace, false, false);

  echo '<ul class="nested" >';

  $cmd = escapeshellcmd("/home/pi/open62541/build/toggle -node $NodeId -namespace $NameSpace -browse opc.tcp://192.168.2.190:4840");
  exec($cmd, $lines);

  $output = print_r($lines, true);
  unset($lines);

  //remove log outputs to reviel JSON Object
  $parts = explode("UA_NODEID", $output);
  for($i=0; $i < count($parts)+1; $i=$i+1) { array_splice( $parts,$i,1); }; //remove every other array items  ()
  $pp= implode(",", $parts);
  $pp = "[$pp]";

  $Browse = json_decode($pp, true);
  foreach($Browse as $Node){
    AddSpan($Node['Id'], $Node['NameSpace'], false, false);
  };

  echo "</ul>";

};

?>



