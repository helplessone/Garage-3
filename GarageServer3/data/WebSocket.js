var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var json = getJson();

var YELLOW = "#ffd11a";
var RED = "#ff0000";
var GREEN = "#009900";
var GRAY = "#999999";
var PURPLE = "#5c00e6"
var WHITE = "#ffffff"
var BLUE = "#0000ff"

var OFFLINE_YELLOW = "#ffeb99";
var OFFLINE_RED = "#ff9999";
var OFFLINE_GREEN = "#99ff99";
var OFFLINE_GRAY = "#cccccc";
var OFFLINE_PURPLE = "#a366ff"
var OFFLINE_BLUE = "#8080ff"

var DEVICE_GARAGE = '1';
var DEVICE_THERMOMETER = '2';

/********* GLOBAL VARIBLES ************/
var timeOffset = 0;
/**************************************/

connection.onopen = function () {
    connection.send("Connect " + new Date());
};
connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
};
connection.onmessage = function (e) {
    console.log("Server: ", e.data);
};
connection.onclose = function(){
    console.log("WebSocket connection closed");
};

setInterval(getJson, 200);

function mainButtonPressed(button) {
	console.log('**** Button Pressed ******');
	console.log ('Button.id = ' + button.id + ' mac = ' + button.getAttribute('mac'));
	connection.send("#" + button.id);
}

function saveSettings() {
	var i;
	console.log("***** saveSettings() ******");
	for (i= 0; i< json.devices.length; i++) {
		var value = document.getElementById("T"+json.devices[i].mac).value;
		var st = "*" + json.devices[i].mac + "?deviceName=" + value;
		connection.send(st);
		console.log(st);
	}
	connection.send('S');
	console.log("Setting saved.");
}

function addDoorButton (deviceName, mac) {

	var b1 = document.createElement("BUTTON"); // Create Button
	if (json.devices.length > 4) b1.style.width = "45%"; else b1.style.width = "95%";
	b1.style.height = "80px";
	b1.style.margin = "5px";
	b1.style.background = "blue";
	b1.style.borderRadius = "10px";
	b1.style.color = "white";
	b1.style.fontSize = "30px";
	b1.id = mac;
	b1.setAttribute('mac', mac);

	// Register click handlers to call respective functions
	b1.onclick = function() {mainButtonPressed(this);};

	// Append them in your DOM i.e to show it on your page.
	// Suppose to append it to an existing element in your page with id as "appp".
	var attachTo = document.getElementById("buttons");
	attachTo.appendChild(b1);
}

function addDoorTextBox (device, textBoxID, container) {
	var br = document.createElement("br");

	if (device.deviceType == DEVICE_GARAGE) {
		var closeMinutes = document.createElement('input'); // closeMinutes to automatically close
		closeMinutes.style.width = '15%';
		closeMinutes.style.height = '20px';
		closeMinutes.style.marginTop = '4px';
		closeMinutes.style.marginLeft = '2px';
		closeMinutes.style.marginRight = '5px';
		closeMinutes.style.textAlign = 'center';
		closeMinutes.style.borderRadius = '10px';
		closeMinutes.style.color = 'black';
		closeMinutes.style.fontSize = '20px';
		closeMinutes.maxLength = 3;
		closeMinutes.id = 'closeMinutes' + device.mac;
		closeMinutes.setAttribute('mac',device.mac);
		closeMinutes.value = '60';

		var closeTime = document.createElement('input'); // time to automatically close
		closeTime.style.width = '20%';
		closeTime.style.height = '20px';
		closeTime.style.marginTop = '4px';
		closeTime.style.marginLeft = '2px';
		closeTime.style.marginRight = '0px';
		closeTime.style.marginBottom = '3px';
		closeTime.style.textAlign = 'center';
		closeTime.style.borderRadius = '10px';
		closeTime.style.color = 'black';
		closeTime.style.fontSize = '20px';
		closeTime.maxLength = 3;
		closeTime.id = 'closeTime' + device.mac;
		closeTime.setAttribute('mac',device.mac);
		closeTime.value = '22:00';
		
		var t1 = document.createTextNode("AUTO CLOSE AFTER ");
		var t2 = document.createTextNode("MIN ");
		var t3 = document.createTextNode("AUTO CLOSE AT ");
	}	
	
	if (device.deviceType == DEVICE_THERMOMETER) {
		var min = document.createElement('input'); // min input
		min.style.width = '15%';
		min.style.height = '20px';
		min.style.marginTop = '4px';
		min.style.marginLeft = '0px';
		min.style.marginRight = '10px';
		min.style.marginBottom = '3px';
		min.style.textAlign = 'center';
		min.style.borderRadius = '10px';
		min.style.color = 'black';
		min.style.fontSize = '20px';
		min.maxLength = 3;
		min.id = 'min' + device.mac;
		min.value = 50;

		var max = document.createElement('input'); // max input
		max.style.width = '15%';
		max.style.height = '20px';
		max.style.marginTop = '4px';
		max.style.marginLeft = '2px';
		max.style.marginRight = '0px';
		min.style.marginBottom = '3px';
		max.style.textAlign = 'center';
		max.style.borderRadius = '10px';
		max.style.color = 'black';
		max.style.fontSize = '20px';
		max.maxLength = 3;
		max.id = 'max' + device.mac;
		max.value = 120;
		
		var celcius = document.createElement('input'); // max input
		celcius.setAttribute("type", "checkbox");
		celcius.style.width = '10%';
		celcius.style.height = '20px';
		celcius.style.marginBottom = '6px';
		celcius.style.marginLeft = '2px';
		celcius.style.marginRight = '0px';
		celcius.style.verticalAlign = 'bottom';
		celcius.style.color = 'black';
		celcius.style.fontSize = '20px';
		celcius.id = 'celcius' + device.mac;
		celcius.value = 120;
		
		var mn = document.createTextNode("MIN ");
		var mx = document.createTextNode("MAX ");
		var cel = document.createTextNode("\xB0C");		
	}

	var tb = document.createElement('input'); // Create Button
	if (device.deviceType == DEVICE_GARAGE) tb.style.width = '95%';
	else if (device.deviceType == DEVICE_THERMOMETER) tb.style.width = '95%';

	tb.style.height = '40px';
	tb.style.marginLeft = '0px';
	tb.style.marginRight = '0px';
	tb.style.borderRadius = '10px';
	tb.style.color = 'black';
	tb.style.fontSize = '24px';
	tb.maxLength = 15;
	tb.id = textBoxID;
	tb.value = device.deviceName;

	container.appendChild(tb);

	if (device.deviceType == DEVICE_GARAGE) {
		container.appendChild(t1);		
		container.appendChild(closeMinutes);
		container.appendChild(t2);
		container.appendChild(br);
		container.appendChild(t3);
		container.appendChild(closeTime);

	}
	if (device.deviceType == DEVICE_THERMOMETER) {
		container.appendChild(mn);		
		container.appendChild(min);
		container.appendChild(mx);
		container.appendChild(max);
		container.appendChild(cel);
		container.appendChild(celcius);
	}

}

function buttonPress(image){
	console.log ('Button press...' + image.getAttribute('mac'));
}

function loadMainControls() {
	var i;
	console.log("Enter loadMainControls");
	var json = getJson();
	if (json !== "null") {
		for (i= 0; i< json.devices.length; i++) {
			addDoorButton(json.devices[i].deviceName, json.devices[i].mac);
		}
	}
	json = getJson();	//reload json to get button text
	console.log("Exit loadMainControls");
}

function timeString(time){
//	console.log("timeOffset = " + timeOffset);
	var timestamp = (time + (timeOffset * 3600)) * 1000; //convert seconds to mSec + offset
	var date = new Date(timestamp);
	
	var month = (date.getMonth()+1).toString();
	var day = date.getDate().toString();
	var hour = date.getHours().toString();
	var minutes = date.getMinutes().toString();
	while (minutes.length < 2) minutes = '0' + minutes;

	var formattedTime = month + '/' + day + ' ' + hour + ':' + minutes;
	return formattedTime;

	return "";
}

function displayTime(){
	var b = document.getElementById('timeButton');
	json = getJson();
	console.log("currTime = " + json.global.currTime);
	b.innerHTML = timeString(json.global.currTime);
}

function incTime(){
	timeOffset++;
	connection.send("@+");
	displayTime();

}

function decTime(){
	timeOffset--;
	connection.send("@-");
	displayTime();
}

function moveUp(button) {
	console.log('Move Up ' + button.id);
	var st = "*" + button.getAttribute("mac") + "?moveDevice=up";
	connection.send(st);
	location.reload();
}

function moveDown(button) {
	console.log('Move Down ' + button.id);
	var st = "*" + button.getAttribute("mac") + "?moveDevice=down";
	connection.send(st);
	location.reload();
}

function deleteDoor(button) {
	var i;
	json = getJson();
	console.log("json.devices.length = " + json.devices.length);
	console.log("*" + button.id + "*");
	for (i=0; i<json.devices.length; i++){
		console.log("|" + json.devices[i].mac + "|");
		console.log("");
		if (json.devices[i].mac === button.getAttribute("mac")) {
			console.log("found it");
			break;
		}
	}
	console.log("i = " + i);
	if (i < json.devices.length) {
		if (json.devices[i].online === 1) {
			window.alert("Unplug device before trying to delete it");
		} else {
			if (window.confirm("Delete device?\nAre you sure?")) {
				console.log('Delete Door ' + button.id);
				var st = "*" + button.getAttribute("mac") + "?deleteDevice=";
				connection.send(st);
				location.reload();
			}
		}
	}
}

function swapSensors(button){
	console.log("swapSensors " + button.id);
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].sensor0 !== json.devices[i].sensor1) { // you can only swap when open or closed
			console.log("sensorSwap = " + json.devices[i].sensorSwap);
			if (json.devices[i].sensorSwap === 0) value = 'TRUE'; else value = 'FALSE';
			var st = "*" + json.devices[i].mac + "?sensorSwap=" + value;

			connection.send(st);
			console.log(st);
			connection.send('S');
			console.log("Setting saved.");
			location.reload();
		}
	}
}

function createSettingsTable(rows) {
	json = getJson();
	var br = document.createElement("br");

	//body reference
	var container = document.getElementById("settingsControls");

	// create elements <table> and a <tbody>
	var tbl = document.createElement("table");
	tbl.style.width = '100%';
	tbl.style.marginLeft = '0px';
	tbl.style.marginRight = '0px';
//	tbl.style.border = "thin solid black";

	var tblBody = document.createElement("tbody");

	// cells creation
	for (var j = 0; j < json.devices.length; j++) {
	// table row creation
		var row = document.createElement("tr");
		row.style.border = "thin solid black";

		for (var i = 0; i < 4; i++) {
			// create element <td> and text node
			//Make text node the contents of <td> element
			// put <td> at end of the table row
			var cell = document.createElement("td");
			cell.style.borderBottom = "2px solid black";

			if (i === 0) {
				addDoorTextBox(json.devices[j], "T" + json.devices[j].mac, cell);
//				cell.borderBottom = 'thick solid black';
			}
			if (i === 1) { //swap sensor button (garage only)
				if (json.devices[j].deviceType == DEVICE_GARAGE) {
					cell.style.width = '50px';
					var b1 = document.createElement('img');
					b1.id = 'swap' + json.devices[j].mac;
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/reverse.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
					b1.style.backgroundColor = getDeviceColor(json.devices[j]);
					b1.onclick = function() {swapSensors(this);};
					if (json.devices[j].sensor0 === json.devices[j].sensor1) { //if door is open or closed allow sensor swap
						b1.style.visibility = 'hidden';
					}
					cell.appendChild(b1);
				}
			}
			if (i === 2) { //delete button
				cell.style.width = '50px';
				if (json.devices[j].online === 0) { // if door if offline, allow delete
					var b1 = document.createElement('img');
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/trash-can.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
					b1.onclick = function() {deleteDoor(this);};
					cell.appendChild(b1);
				}
			}
			if (i === 3) { //move buttons
				cell.style.width = '50px';
				var b1 = document.createElement('img');
				b1.setAttribute("mac",json.devices[j].mac);
				b1.src = '/arrow-up.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {moveUp(this);};
				cell.appendChild(b1);

				cell.appendChild(br);

				var b2 = document.createElement('img');
				b2.setAttribute("mac",json.devices[j].mac);
				b2.src = '/arrow-up.png';
				b2.style.width = '30px';
				b2.style.height = '30px';
				b2.style.transform = 'rotate(180deg)';
				b2.onclick = function() {moveDown(this);};
				cell.appendChild(b2);
			}

			row.appendChild(cell);
		}

		//row added to end of table body
		tblBody.appendChild(row);
	}

	// append the <tbody> inside the <table>
	tbl.appendChild(tblBody);
	// put <table> in the <settingsControls>
	container.appendChild(tbl);
	// tbl border attribute to
//	tbl.setAttribute("border", "0");

	var b1 = document.createElement('img');
	b1.src = '/reverse.png';
	b1.style.width = '30px';
	b1.style.height = '30px';
	b1.style.vericalAlign = 'bottom';

	var s = document.createElement('span');
	s.style.fontSize = '18px';
	var txt = document.createTextNode(" = Switch Door Position");
	s.appendChild(b1);
	s.appendChild(txt);
	container.appendChild(s);
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")

	createSettingsTable(json.devices.length);
	displayTime();
}

function getDeviceColor(device){
	var sensors = (device.online*100) + (device.sensor0*10) + device.sensor1;
	if (device.sensorSwap === 0) {
		switch (sensors) {
			//online values
			case 101: return RED; break;
			case 111: return YELLOW; break;
			case 110: return GREEN; break;
			case 100: return PURPLE; break;
			//offline values
			case 10: return OFFLINE_RED; break;
			case 1: return OFFLINE_YELLOW; break;
			case 1: return OFFLINE_GREEN; break;
			case 0: return OFFLINE_PURPLE; break;
		}
	} else {
		switch (sensors) {
			//online values
			case 110: return RED; break;
			case 111: return YELLOW; break;
			case 101: return GREEN; break;
			case 100: return PURPLE; break;
			//offline values
			case 10: return OFFLINE_RED; break;
			case 11: return OFFLINE_YELLOW; break;
			case 1: return OFFLINE_GREEN; break;
			case 0: return OFFLINE_PURPLE; break;
		}
	}
}

function getJson(){
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);
//    console.log("Response " + Httpreq.responseText);

    if (Httpreq.responseText !== "null") {
//		console.log(Httpreq.responseText);
		json = JSON.parse(Httpreq.responseText);
		
		currTime = json.global.currTime;
		timeOffset = json.global.timeOffset;

		var i;
		for (i=0; i<json.devices.length; i++) {
			var button = document.getElementById(json.devices[i].mac);
			if (button != null) {
				if (json.devices[i].deviceType == DEVICE_GARAGE) {
					button.style.backgroundColor = getDeviceColor(json.devices[i]);
					var st;
					var color = getDeviceColor(json.devices[i]);
					switch (color) {
						case RED: st = "Open since "; break;
						case GREEN: st = "Closed since "; break;
						case YELLOW: st = "Partially Open since "; break;
						case PURPLE: st = "Invalid State since "; break;
						default: st = "Offline since ";
					}
					var deviceTime = timeString(json.devices[i].deviceTime);

					button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + " " + deviceTime + "</p>";
				}
				if (json.devices[i].deviceType == DEVICE_THERMOMETER) {
					if (json.devices[i].online == 1) button.style.backgroundColor = GREEN;
					else button.style.backgroundColor = OFFLINE_GREEN;

					var st = "Last Read ";
					var deviceTime = timeString(json.devices[i].deviceTime);
					var degC = Math.round(json.devices[i].sensor0 / 100) + "\xB0 C";
					var degF = Math.round( ((json.devices[i].sensor0 / 100) * 9/5) + 32) + "\xB0 F";
					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + degC + ' / ' + degF + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
				}
			}
			var swapbutton = document.getElementById('swap' + json.devices[i].mac);
			if (swapbutton != null) {
				if (json.devices[i].sensor0 === json.devices[i].sensor1) { //if door is open or closed alow sensor swap
					swapbutton.style.visibility = 'hidden';
				} else swapbutton.style.visibility = 'visible';
				swapbutton.style.backgroundColor = getDeviceColor(json.devices[i]);
			}
		}
		return json;
	}
    return;
}

/* When the user clicks on the button,
toggle between hiding and showing the dropdown content */
function myFunction() {
  document.getElementById("myDropdown").classList.toggle("show");
}

// Close the dropdown menu if the user clicks outside of it
window.onclick = function(event) {
  if (!event.target.matches('.dropbtn')) {
    var dropdowns = document.getElementsByClassName("dropdown-content");
    var i;
    for (i = 0; i < dropdowns.length; i++) {
      var openDropdown = dropdowns[i];
      if (openDropdown.classList.contains('show')) {
        openDropdown.classList.remove('show');
      }
    }
  }
}

