<!DOCTYPE html>
<html lang="en">
<head>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>

<title>Device Control</title>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
* {
  box-sizing: border-box;
}

/* Style the body */
body {
  font-family: Arial, Helvetica, sans-serif;
  margin: 0;
}

/* Header/logo Title */
.header {
  padding: 5px;
  text-align: center;
  background: #1abc9c;
  color: white;
}

/* Increase the font size of the heading */
.header h1 {
  font-size: 30px;
}

/* Style the top navigation bar */
.navbar {
  overflow: hidden;
  background-color: #333;
}

/* Style the navigation bar links */
.navbar a {
  float: left;
  display: block;
  color: white;
  text-align: center;
  padding: 14px 20px;
  text-decoration: none;
}

/* Right-aligned link */
.navbar a.right {
  float: right;
}

/* Change color on hover */
.navbar a:hover {
  background-color: #ddd;
  color: black;
}

/* Column container */
.row {  
  display: -ms-flexbox; /* IE10 */
  display: flex;
  -ms-flex-wrap: wrap; /* IE10 */
  flex-wrap: wrap;
}

/* Create two unequal columns that sits next to each other */
/* Sidebar/left column */
.side {
  -ms-flex: 30%; /* IE10 */
  flex: 30%;
  background-color: #f1f1f1;
  padding: 20px;
}

/* Main column */
.main {   
  -ms-flex: 70%; /* IE10 */
  flex: 70%;
  background-color: white;
  padding: 20px;
}

/* Fake image, just for this example */
.fakeimg {
  background-color: #aaa;
  width: 100%;
  padding: 20px;
}

/* Footer */
.footer {
  padding: 20px;
  text-align: center;
  background: #ddd;
}

/* Responsive layout - when the screen is less than 700px wide, make the two columns stack on top of each other instead of next to each other */
@media screen and (max-width: 700px) {
  .row {   
    flex-direction: column;
  }
}

/* Responsive layout - when the screen is less than 400px wide, make the navigation links stack on top of each other instead of next to each other */
@media screen and (max-width: 400px) {
  .navbar a {
    float: none;
    width: 100%;
  }
}

ul, #myUL {
  list-style-type: none;
}

#myUL {
  margin: 0;
  padding: 0;
}

.box {
  cursor: pointer;
  -webkit-user-select: none; /* Safari 3.1+ */
  -moz-user-select: none; /* Firefox 2+ */
  -ms-user-select: none; /* IE 10+ */
  user-select: none;
}

.box::before {
  content: "\2192 \2610";
//  content: "\002B";
  //  content: "\2610";
  color: black;
  display: inline-block;
  margin-right: 6px;
}

.check-box::before {
  content: "\2193 \2611";
//  content: "\2212";
//  content: "\2611";
  color: dodgerblue;
}

.nested {
  display: none;
}

.active {
  display: block;
}

table {
  font-family: arial, sans-serif;
  border-collapse: collapse;
  width: 100%;
}

td, th {
  border: 1px solid #dddddd;
  text-align: left;
  padding: 8px;
}

tr:nth-child(even) {
  background-color: #dddddd;
}

</style>

</head>
<body>

<div class="header">
  <p id="time" class="right">Loading</p>
<script>
var myVar = setInterval(myTimer, 1000);

function myTimer() {
  var d = new Date();
  var t = d.toLocaleTimeString();
  document.getElementById("time").innerHTML = t;
}
</script>
</div>
<div class="row">
  <div class="side">
    <h2>Devices</h2>

<ul id="myUL">
  <li id='85' namespace='0'><span class='box'>Objects</span></li>
</ul>

<script>
$(document).ready(function(){

  function AddHandler(){
    var toggler = document.getElementsByClassName("box");
    var i;

    for (i = 0; i < toggler.length; i++) {
      toggler[i].addEventListener("click", ClickedMe);
    };
  };

function Changed(){
//  alert("Change finished");
};

  function SettingChanges(){
    var box_ns = this.getAttributeNode('ns').value;
    var box_node = this.getAttributeNode('node').value;
    var box_value = this.checked;
    var old_id = this.id;

    //need to see if the event handling can be fixed
    this.id = "qwertyuio";
    $("#"+this.id).load("gpio.php", {nodeid: box_node, ns : box_ns, value : box_value}, Changed); 
    this.id = old_id;
  };

  function loaded(){
    this.children[1].classList.toggle("active");
    AddHandler();
    var Node = this.children[0].children[0];

    var table = document.getElementById("refreshresult");
    var clnTab = table.cloneNode(true);
    clnTab.id ="";

    var itm = document.getElementById("tdexample");
    var cln = itm.cloneNode(true);
    cln.id ="";
    
    cln.children[0].innerHTML = "Time Stamp";
    var d = new Date();
    var t = d.toLocaleString();
    cln.children[1].innerHTML = t;
    clnTab.append(cln);

    if( Node.attributes['type'].value = 'boolean'){
        var itm = document.getElementById("tdexample");
        var cln = itm.cloneNode(true);
        cln.id = "";
        cln.children[0].innerHTML = "Change Setting";
        var para = document.createElement("INPUT");  
        Checked = (Node.attributes['current'].value == 1  ? " checked" : "");
        box_id = Node.parentElement.id;
        box_ns = Node.parentElement.getAttributeNode('namespace').value;        
        cln.children[1].innerHTML ="<input type=\"checkbox\" node=\""+ box_id +"\"  ns=\""+ box_ns +"\" " + Checked + ">";
        cln.children[1].children[0].addEventListener("click", SettingChanges);

        clnTab.append(cln);
    };

    var attrs = Node.attributes;
    for(var i = attrs.length - 1; i >= 0; i--) {
      var itm = document.getElementById("tdexample");
      var cln = itm.cloneNode(true);
      cln.id = "";
      cln.children[0].innerHTML = attrs[i].name;
      cln.children[1].innerHTML = attrs[i].value;
      clnTab.append(cln);
    };

    var mark = document.getElementById("newResult");
    mark.insertAdjacentElement("afterend", clnTab);
    
  };

function ClickedMe(){
    var id = this.parentElement.id;
    var ns = this.parentElement.getAttributeNode('namespace').value;
    this.innerHTML += " Loading...";
    $("#"+id).load("gpio.php", {nodeid: id, ns : ns}, loaded); 
};

AddHandler();

});

</script>
  </div>
  <div class="main">
    <h2>Details</h2>
    <h5 id="out"></h5>
    <div id="newResult"></div>
    <table id="refreshresult">
      <th>Attribute</th>
      <th>Value</th>
    </tr>
    <tr id="tdexample">
      <td class="tdname"></td>
      <td class="tdvale"></td>
    </tr>
    </table>
|    <br>
  </div>
</div>
<div class="navbar">
  <a href="#">Link</a>
  <a href="#" class="right">Account</a>
</div>
<div class="footer">
  <p>Distributed Control System</p>
</div>
</body>
</html>
