var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

var settingsColumn = ['0px','0px','0px'];
var screenWidth = 410;

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
var DEVICE_LATCH = '4';
var DEVICE_BAT = '6';
var DEVICE_CURTAIN = '7';

/********* GLOBAL VARIBLES ************/
var timeOffset = 0;
/**************************************/

connection.onopen = function () {
	connection.send("Connect " + new Date());
	console.log("connection.onopen");
};
connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
};
connection.onmessage = function (e) {
	console.log ("Received pushed json");
//	console.log(e.data);
	processJson(e.data);
};
connection.onclose = function(){
    console.log("WebSocket connection closed");
};

setInterval(getIntervalJson, 10000); // every 10 seconds

function deviceButtonPressed(button) {
	console.log('**** Button Pressed ******')
	console.log ('Button.id = ' + button.id + ' mac = ' + button.getAttribute('mac'))
	connection.send("#" + button.id)
}

function getDeviceJson(device) {
	devicejson = {};

	devicejson['mac'] = device.mac;
	devicejson['deviceName'] = document.getElementById("DEVICENAME"+device.mac).value;
	if (device.deviceType == DEVICE_GARAGE) {
		devicejson['closeDelay'] = parseInt(document.getElementById("CLOSEDELAY"+device.mac).value); 
		
		var value = document.getElementById("CLOSETIME"+device.mac).value

		value = value.replace(/\D/g, '');
		console.log("closeTime = " + parseInt(value));

		devicejson['closeTime'] = parseInt(value); 
	}
	if (device.deviceType == DEVICE_THERMOMETER) {
		devicejson['minTemp'] = parseInt(document.getElementById("MINTEMP"+device.mac).value); 
		devicejson['maxTemp'] = parseInt(document.getElementById("MAXTEMP"+device.mac).value); 
		devicejson['celcius'] = document.getElementById("CELCIUS"+device.mac).checked?1:0;
	}
	if (device.deviceType == DEVICE_BAT) {
		devicejson['startTime'] = parseInt(document.getElementById("MOTIONSTART"+device.mac).value); 
		devicejson['stopTime'] = parseInt(document.getElementById("MOTIONSTOP"+device.mac).value); 
	}
	if (device.deviceType == DEVICE_CURTAIN) {
		devicejson['upTime'] = parseInt(document.getElementById("UPTIME"+device.mac).value); 
		devicejson['downTime'] = parseInt(document.getElementById("DOWNTIME"+device.mac).value); 
		devicejson['rotationCount'] = parseInt(document.getElementById("ROTATIONCOUNT"+device.mac).value); 
	}		
	return devicejson;
}

function getGlobalsJson() {
	globalsjson = {};
	globalsjson['timeOffset'] = timeOffset;
	return globalsjson;
}

function saveGlobals() {
	connection.send('^' + JSON.stringify(getGlobalsJson()));	
}

function saveSettings() {
	var i;
	console.log("***** saveSettings() ******");
	for (i= 0; i< json.devices.length; i++) {
		var st = '^' + JSON.stringify(getDeviceJson(json.devices[i]));
		console.log(st);	
		connection.send(st);
	}
	saveGlobals();
	console.log("Setting saved.");
}

function deleteSettings() {
	connection.send('D');
	console.log("Setting deleted");	
}

function addDeviceButton (deviceName, mac) {
	var b1 = document.createElement("BUTTON"); // Create Button
	if (json.devices.length > 4) b1.style.width = "95%"; else b1.style.width = "95%";
	b1.style.height = "80px"; //was 80
	b1.style.margin = "5px";
	b1.style.background = "blue";
	b1.style.borderRadius = "10px";
	b1.style.color = "white";
	b1.style.fontSize = "30px";
	b1.id = mac;
	b1.setAttribute('mac', mac);

	// Register click handlers to call respective functions
	b1.onclick = function() {deviceButtonPressed(this);};

	// Append them in your DOM i.e to show it on your page.
	// Suppose to append it to an existing element in your page with id as "appp".
	var attachTo = document.getElementById("buttons");
	attachTo.appendChild(b1);
}

function downButton(button){
	console.log("downButton " + button.id);
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].deviceType == DEVICE_CURTAIN) {
			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['downButton'] = 0;	//Value doesn't matter
			var st = JSON.stringify(devicejson);
			console.log("downButton json = " + st);
			connection.send('^' + st);		//send update to arduino server						
		}	
	}	
}

function upButton(button){
	console.log("upButton " + button.id);
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].deviceType == DEVICE_CURTAIN) {
			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['upButton'] = 0;	//Value doesn't matter
			var st = JSON.stringify(devicejson);
			console.log("upButton json = " + st);
			connection.send('^' + st);		//send update to arduino server						
		}	
	}	
}

function stopButton(button){
	console.log("stopButton " + button.id);
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].deviceType == DEVICE_CURTAIN) {
			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['stopButton'] = 0;	//Value doesn't matter
			var st = JSON.stringify(devicejson);
			console.log("stopButton json = " + st);
			connection.send('^' + st);		//send update to arduino server						
		}	
	}	
}

function setBottom(button){
	console.log("setBottom " + button.id);
	json = getJson();
	var i;
	for (i=0; i<json.devices.length; i++){
		if (json.devices[i].mac === button.getAttribute("mac")) {
			break;
		}
	}
	if (i < json.devices.length) {
		if (json.devices[i].deviceType == DEVICE_CURTAIN) {
			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['setBottom'] = 0;	//Value doesn't matter
			var st = JSON.stringify(devicejson);
			console.log("setBottom json = " + st);
			connection.send('^' + st);		//send update to arduino server						
		}	
	}
}

function addDoorTextBox (device, container) {
	var br = document.createElement("br");

	if (device.deviceType == DEVICE_GARAGE) {
		var closeDelay = document.createElement('input'); // closeDelay to automatically close
		closeDelay.style.width = '40px';
		closeDelay.style.height = '20px';
		closeDelay.style.marginTop = '4px';
		closeDelay.style.marginLeft = '5px';
		closeDelay.style.marginRight = '5px';
		closeDelay.style.textAlign = 'center';
		closeDelay.style.borderRadius = '10px';
		closeDelay.style.color = 'black';
		closeDelay.style.fontSize = '20px';
		closeDelay.maxLength = 3;
		closeDelay.id = 'CLOSEDELAY' + device.mac;
		closeDelay.setAttribute('mac',device.mac);
		closeDelay.value = device.closeDelay;

		var closeTime = document.createElement('input'); // time to automatically close
		closeTime.style.width = '45px';
		closeTime.style.height = '20px';
		closeTime.style.marginTop = '4px';
		closeTime.style.marginLeft = '5px';
		closeTime.style.marginRight = '5px';
		closeTime.style.marginBottom = '3px';
		closeTime.style.textAlign = 'center';
		closeTime.style.borderRadius = '10px';
		closeTime.style.color = 'black';
		closeTime.style.fontSize = '20px';
		closeTime.maxLength = 5;
		closeTime.id = 'CLOSETIME' + device.mac;
		closeTime.setAttribute('mac',device.mac);
		closeTime.value = device.closeTime;
		
		var t0 = document.createTextNode("");
		var t1 = document.createTextNode("Auto Close After");
		var t2 = document.createTextNode("Minutes");
		var t3 = document.createTextNode("Auto Close at");
		var t4 = document.createTextNode("(9pm=2100)");
	}	

	
	if (device.deviceType == DEVICE_THERMOMETER) {
		var min = document.createElement('input'); // min input		
		min.style.width = '40px';
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
		min.id = 'MINTEMP' + device.mac;
		min.value = device.minTemp;

		var max = document.createElement('input'); // max input
		max.style.width = '40px';
		max.style.height = '20px';
		max.style.marginTop = '4px';
		max.style.marginLeft = '2px';
		max.style.marginRight = '10px';
		max.style.marginBottom = '3px';
		max.style.textAlign = 'center';
		max.style.borderRadius = '10px';
		max.style.color = 'black';
		max.style.fontSize = '20px';
		max.maxLength = 3;
		max.id = 'MAXTEMP' + device.mac;
		max.value = device.maxTemp;
		
		var celcius = document.createElement('input'); // celcius check box
		celcius.setAttribute("type", "checkbox");
		celcius.style.width = '15px';
		celcius.style.height = '20px';
		celcius.style.marginBottom = '6px';
		celcius.style.marginLeft = '2px';
		celcius.style.marginRight = '0px';
		celcius.style.verticalAlign = 'bottom';
		celcius.style.color = 'black';
		celcius.style.fontSize = '20px';
		celcius.id = 'CELCIUS' + device.mac;
		celcius.checked = (device.celcius == 1)?true:false;
		
		var mn = document.createTextNode("Min ");
		var mx = document.createTextNode("Max ");
		var cel = document.createTextNode("\xB0C");
	}

	if (device.deviceType == DEVICE_BAT) {
		var start = document.createElement('input'); // start time		
		start.style.width = '45px';
		start.style.height = '20px';
		start.style.marginTop = '4px';
		start.style.marginLeft = '5px';
		start.style.marginRight = '5px';
		start.style.marginBottom = '3px';
		start.style.textAlign = 'center';
		start.style.borderRadius = '10px';
		start.style.color = 'black';
		start.style.fontSize = '20px';
		start.maxLength = 5;
		start.id = 'MOTIONSTART' + device.mac;
		start.value = device.startTime;

		var stop = document.createElement('input'); // stop time
		stop.style.width = '45px';
		stop.style.height = '20px';
		stop.style.marginTop = '4px';
		stop.style.marginLeft = '5px';
		stop.style.marginRight = '5px';
		stop.style.marginBottom = '3px';
		stop.style.textAlign = 'center';
		stop.style.borderRadius = '10px';
		stop.style.color = 'black';
		stop.style.fontSize = '20px';
		stop.maxLength = 5;
		stop.id = 'MOTIONSTOP' + device.mac;
		stop.value = device.stopTime;
		
		var starttext = document.createTextNode("Start Time ");
		var stoptext = document.createTextNode("Stop Time");
	}

	if (device.deviceType == DEVICE_CURTAIN) {
		var uptime = document.createElement('input'); // up time		
		uptime.style.width = '45px';
		uptime.style.height = '20px';
		uptime.style.marginTop = '4px';
		uptime.style.marginLeft = '5px';
		uptime.style.marginRight = '5px';
		uptime.style.marginBottom = '3px';
		uptime.style.textAlign = 'center';
		uptime.style.borderRadius = '10px';
		uptime.style.color = 'black';
		uptime.style.fontSize = '20px';
		uptime.maxLength = 5;
		uptime.id = 'UPTIME' + device.mac;
		uptime.value = device.upTime;

		var downtime = document.createElement('input'); // down time
		downtime.style.width = '45px';
		downtime.style.height = '20px';
		downtime.style.marginTop = '4px';
		downtime.style.marginLeft = '5px';
		downtime.style.marginRight = '5px';
		downtime.style.marginBottom = '3px';
		downtime.style.textAlign = 'center';
		downtime.style.borderRadius = '10px';
		downtime.style.color = 'black';
		downtime.style.fontSize = '20px';
		downtime.maxLength = 5;
		downtime.id = 'DOWNTIME' + device.mac;
		downtime.value = device.downTime;

		var rotations = document.createElement('input'); // rotation count
		rotations.style.width = '45px';
		rotations.style.height = '20px';
		rotations.style.marginTop = '4px';
		rotations.style.marginLeft = '5px';
		rotations.style.marginRight = '5px';
		rotations.style.marginBottom = '3px';
		rotations.style.textAlign = 'center';
		rotations.style.borderRadius = '10px';
		rotations.style.color = 'black';
		rotations.style.fontSize = '20px';
		rotations.maxLength = 3;
		rotations.id = 'ROTATIONCOUNT' + device.mac;
		rotations.value = device.rotationCount;

		var downbutton = document.createElement('img'); // rotation count
		downbutton.src =  'https://api.iconify.design/emojione:fast-down-button.svg?color=%230099ff';
		downbutton.style.width = '40px';
		downbutton.style.height = '40px';
		downbutton.style.marginTop = '6px';
		downbutton.style.marginLeft = '5px';
		downbutton.style.marginRight = '5px';
		downbutton.style.marginBottom = '3px';
		downbutton.style.textAlign = 'center';
		downbutton.style.borderRadius = '5px';
		downbutton.id = 'DOWNBUTTON' + device.mac;
		downbutton.setAttribute("mac",device.mac);
		downbutton.onclick = function() {downButton(this);};		

		var upbutton = document.createElement('img'); // rotation count
		upbutton.src =  'https://api.iconify.design/emojione:fast-up-button.svg?color=%230099ff';
		upbutton.style.width = '40px';
		upbutton.style.height = '40px';
		upbutton.style.marginTop = '6px';
		upbutton.style.marginLeft = '5px';
		upbutton.style.marginRight = '5px';
		upbutton.style.marginBottom = '3px';
		upbutton.style.textAlign = 'center';
		upbutton.style.borderRadius = '5px';
		upbutton.id = 'UPBUTTON' + device.mac;
		upbutton.setAttribute("mac",device.mac);
		upbutton.onclick = function() {upButton(this);};

		var stopbutton = document.createElement('img'); // rotation count
		stopbutton.src =  'https://api.iconify.design/emojione:stop-button.svg?color=%230099ff';
		stopbutton.style.width = '40px';
		stopbutton.style.height = '40px';
		stopbutton.style.marginTop = '6px';
		stopbutton.style.marginLeft = '5px';
		stopbutton.style.marginRight = '5px';
		stopbutton.style.marginBottom = '3px';
		stopbutton.style.textAlign = 'center';
		stopbutton.style.borderRadius = '5px';
		stopbutton.id = 'STOPBUTTON' + device.mac;
		stopbutton.setAttribute("mac",device.mac);
		stopbutton.onclick = function() {stopButton(this);};

		var setbottom = document.createElement('img'); // rotation count
		setbottom.src =  'https://api.iconify.design/bx:bxs-arrow-to-bottom.svg?color=%230099ff';
		setbottom.style.width = '40px';
		setbottom.style.height = '40px';
		setbottom.style.marginTop = '6px';
		setbottom.style.marginLeft = '5px';
		setbottom.style.marginRight = '5px';
		setbottom.style.marginBottom = '3px';
		setbottom.style.textAlign = 'center';
		setbottom.style.borderRadius = '5px';
		setbottom.id = 'SETBOTTOM' + device.mac;
		setbottom.setAttribute("mac",device.mac);
		setbottom.onclick = function() {setBottom(this);};

		var downtimetext = document.createTextNode("Lower @ ");
		var uptimetext = document.createTextNode("Raise @ ");
		var rotationtext = document.createTextNode("Rotations ")
	}

	if (device.deviceType == DEVICE_THERMOMETER) {
		var icon_therm = document.createElement('img');
//		icon_therm.src =  'https://api.iconify.design/openmoji:thermometer.svg';
		icon_therm.src =  'https://api.iconify.design/ion:thermometer-outline.svg?color=%230099ff';
		icon_therm.style.width = '30px';
		icon_therm.style.height = '30px';
		icon_therm.style.verticalAlign = '-5px';
		icon_therm.style.backgroundColor = WHITE;
		container.appendChild(icon_therm);
	}
	if (device.deviceType == DEVICE_GARAGE) {
		var icon_house = document.createElement('img');
		icon_house.src =  'https://api.iconify.design/mdi:garage.svg?color=%230099ff';
//		https://api.iconify.design/ion:home-outline.svg';
		icon_house.style.width = '30px';
		icon_house.style.height = '30px';
		icon_house.style.verticalAlign = '-5px';
		icon_house.style.backgroundColor = WHITE;
		container.appendChild(icon_house);
		
	}	
	if (device.deviceType == DEVICE_LATCH) {
		var icon_latch = document.createElement('img');
//		icon_latch.src =  'https://api.iconify.design/noto:open-mailbox-with-raised-flag.svg';
		icon_latch.src =  'https://api.iconify.design/mdi:electric-switch.svg?color=%230099ff';

//		https://api.iconify.design/emojione-monotone:closed-mailbox-with-raised-flag.svg';
		icon_latch.style.width = '30px';
		icon_latch.style.height = '30px';
		icon_latch.style.verticalAlign = '-5px';
		icon_latch.style.backgroundColor = WHITE;
		container.appendChild(icon_latch);
	}

	if (device.deviceType == DEVICE_BAT) {
		var icon_bat = document.createElement('img');
//		icon_bat.src =  'https://api.iconify.design/noto:bat.svg';
		icon_bat.src =  'https://api.iconify.design/mdi:motion-sensor.svg?color=%230099ff';		
		icon_bat.style.width = '30px';
		icon_bat.style.height = '30px';
		icon_bat.style.verticalAlign = '-5px';
		icon_bat.style.backgroundColor = WHITE;
		icon_bat.setAttribute("mac",device.mac);
		icon_bat.onclick = function() {
			pirSettings(icon_bat);
		};	
		container.appendChild(icon_bat);
	}
	
	if (device.deviceType == DEVICE_CURTAIN) {
		var icon_curtain = document.createElement('img');
		icon_curtain.src =  'https://api.iconify.design/emojione-monotone:bat.svg?color=%230099ff';			
		icon_curtain.style.width = '30px';
		icon_curtain.style.height = '30px';
		icon_curtain.style.verticalAlign = '-5px';
		icon_curtain.style.backgroundColor = WHITE;
		icon_curtain.setAttribute("mac",device.mac);
		container.appendChild(icon_curtain);
	}	

	var tb = document.createElement('input'); // Create Text Box
	if (device.deviceType == DEVICE_GARAGE) tb.style.width = screenWidth - 140 + 'px'; //"150px"; //220px";  //container.style.width - b1.style.width; //  '95%';
	if (device.deviceType == DEVICE_THERMOMETER) tb.style.width = screenWidth - 140 + 'px'; //'95%';
	if (device.deviceType == DEVICE_LATCH) tb.style.width = screenWidth - 140 + 'px';
	if (device.deviceType == DEVICE_BAT) tb.style.width = screenWidth - 140 + 'px';
	if (device.deviceType == DEVICE_CURTAIN) tb.style.width = screenWidth - 140 + 'px';

	tb.style.height = '40px';
	tb.style.marginTop = '2px';
	tb.style.marginLeft = '3px';
	tb.style.marginRight = '0px';
	tb.style.borderRadius = '10px';
	tb.style.color = 'black';
	tb.style.fontSize = '24px';
	tb.maxLength = 15;
	tb.id = 'DEVICENAME' + device.mac;
	tb.value = device.deviceName;

	container.appendChild(tb);

	if (device.deviceType == DEVICE_GARAGE) {
		
		container.appendChild(t0);
		container.appendChild(br);
		container.appendChild(t1);		
		container.appendChild(closeDelay);
		container.appendChild(t2);
		container.appendChild(br);
		container.appendChild(t3);
		container.appendChild(closeTime);
		container.appendChild(t4);
	}
	if (device.deviceType == DEVICE_THERMOMETER) {
		container.appendChild(br);
		container.appendChild(mn);		
		container.appendChild(min);
		container.appendChild(mx);
		container.appendChild(max);
		container.appendChild(cel);
		container.appendChild(celcius);
	}
	if (device.deviceType == DEVICE_BAT) {
		container.appendChild(br);
		container.appendChild(starttext);		
		container.appendChild(start);
		container.appendChild(stoptext);
		container.appendChild(stop);
	}
	if (device.deviceType == DEVICE_CURTAIN) {
		container.appendChild(br);
		container.appendChild(downtimetext);		
		container.appendChild(downtime);
		container.appendChild(uptimetext);
		container.appendChild(uptime);
		container.appendChild(br);
		container.appendChild(rotationtext);
		container.appendChild(rotations);
		container.appendChild(br);
		container.appendChild(downbutton);	
		container.appendChild(stopbutton);	
		container.appendChild(upbutton);	
		container.appendChild(setbottom);	
	}	
}



function loadMainControls() {
	var i;
	console.log("loadControls()")
	setScreenWidth();
	var json = getJson();
	console.log(json);
	console.log("****** loadMainControls() *************");
	console.log(json);
	if (json !== "null") {
		for (i= 0; i< json.devices.length; i++) {
			addDeviceButton(json.devices[i].deviceName, json.devices[i].mac);
		}
	}
	json = getJson();	//reload json to get button text
}
/*
function loadMainControls2() {
	bloadControls = true;
}
*/
function timeString(time, withSeconds) {

	var currDate = new Date(time * 1000);
	currDate.setHours(currDate.getHours() + timeOffset);

	var month = (currDate.getUTCMonth()+1).toString();
	var day = currDate.getUTCDate().toString();
	var hour = currDate.getUTCHours().toString();
	var minutes = currDate.getUTCMinutes().toString();
	var sec = currDate.getUTCSeconds().toString();
	while (minutes.length < 2) minutes = '0' + minutes;
	while (hour.length < 2) hour = '0' + hour;
	while (sec.length < 2) sec = '0' + sec;

	var formattedTime = month + '/' + day + ' ' + hour + ':' + minutes;
	if (withSeconds === 'true') formattedTime = formattedTime + ':' + sec;

	return formattedTime;
}

function displayTime(){
	var b = document.getElementById('timeButton');
	json = getJson();
	b.innerHTML = timeString(json.global.currTime);
}

function incTime(){
	timeOffset++;
	saveGlobals();
	displayTime();

}

function decTime(){
	timeOffset--;
	saveGlobals();
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
		if (json.devices[i].deviceType == DEVICE_GARAGE) {
			if (json.devices[i].sensor0 !== json.devices[i].sensor1) { // you can only swap when open or closed
				console.log("sensorSwap = " + json.devices[i].sensorSwap);
				if (json.devices[i].sensorSwap === 0) value = 1; else value = 0;

				devicejson = {};
				devicejson['mac'] = json.devices[i].mac;
				devicejson['sensorSwap'] = value;	
				var st = JSON.stringify(devicejson);
				console.log("sensorSwap json = " + st);
				connection.send('^' + st);
			}
		}
		if (json.devices[i].deviceType == DEVICE_LATCH) {
				devicejson = {};
				devicejson['mac'] = json.devices[i].mac;
				devicejson['switchReverse'] = 0;	//Value doesn't matter, arduino just inverts switchReverse
				var st = JSON.stringify(devicejson);
				console.log("switchReverse json = " + st);
				connection.send('^' + st);		//send update to arduino server
//				json = getJson();
//				connection.send("#" + json.devices[i].mac);  //press button to reset latch							
		}
		if (json.devices[i].deviceType == DEVICE_CURTAIN) {
			devicejson = {};
			devicejson['mac'] = json.devices[i].mac;
			devicejson['motorReverse'] = 0;	//Value doesn't matter, arduino just inverts motorReverse
			var st = JSON.stringify(devicejson);
			console.log("motorReverse json = " + st);
			connection.send('^' + st);		//send update to arduino server						
		}	
	}	
}

function createSettingsTable() {
	json = getJson();
	//body reference
	var container = document.getElementById("settingsControls");
	var br = document.createElement("br");

	// cells creation
	for (var j = 0; j < json.devices.length; j++) {
		var tbl = document.createElement("table"); //create new table for each device
		tbl.style.width = '100%';
		tbl.style.marginLeft = '0px';
		tbl.style.marginRight = '0px';
		tbl.style.border = "thin solid black";
		var tblBody = document.createElement("tbody");

		// table row creation
		var row = document.createElement("tr");
//		row.style.width = '100%';
//		row.className = 'settingsTableRow';

		for (var i = 0; i < 3; i++) {
			// create element <td> and text node
			//Make text node the contents of <td> element
			// put <td> at end of the table row
			var cell = document.createElement("td");
//			cell.className = 'settingsTableCell';

			if (i === 0) {
				cell.style.width = settingsColumn[0];
				addDoorTextBox(json.devices[j], cell);
				cell.appendChild(br);
			}
			if (i === 1) { //swap sensor button (garage/latch/curtain)				
				cell.style.width = settingsColumn[1]; 
				if (json.devices[j].deviceType == DEVICE_GARAGE  || json.devices[j].deviceType == DEVICE_LATCH || json.devices[j].deviceType == DEVICE_CURTAIN) {
					var b1 = document.createElement('img');
					b1.id = 'swap' + json.devices[j].mac;
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src =  '/reverse.png';
					b1.style.width = '30px';
					b1.style.height = '30px';
					b1.style.backgroundColor = getDeviceColor(json.devices[j]);
					b1.onclick = function() {swapSensors(this);};
					if (json.devices[j].deviceType === DEVICE_GARAGE)
					  if (json.devices[j].sensor0 === json.devices[j].sensor1) b1.style.visibility = 'hidden';  //if door is open or closed allow sensor swap
					if (json.devices[j].deviceType === DEVICE_LATCH) b1.style.visibility = 'visible';  //LATCH switch invert
						
					cell.appendChild(b1);
				}
				
				if (json.devices[j].online === 0) { // if door if offline, allow delete
					var b1 = document.createElement('img');
					b1.setAttribute("mac",json.devices[j].mac);
					b1.src = '/trash-can.png';
					b1.style.width = '30px'; //'30px';
					b1.style.height = '30px'; //'30px';
					b1.onclick = function() {deleteDoor(this);};
					cell.appendChild(br);
					cell.appendChild(b1);
				}				
			}
			
			if (i === 2) { //move buttons
				cell.style.width = settingsColumn[2]; 
				var b1 = document.createElement('img');
				b1.setAttribute("mac",json.devices[j].mac);
				b1.src = '/arrow-up.png';
				b1.style.width = '30px';
				b1.style.height = '30px';
				b1.onclick = function() {moveUp(this);};
				cell.appendChild(b1);

				cell.appendChild(br);

				var b2 = document.createElement('img');
				cell.appendChild(b2);
				b2.setAttribute("mac",json.devices[j].mac);
				b2.src = '/arrow-up.png';
				b2.style.width = '30px';
				b2.style.height = '30px';
				b2.style.transform = 'rotate(180deg)';
				b2.onclick = function() {moveDown(this);};
			}

			row.appendChild(cell);
		}

		//row added to end of table body
		tblBody.appendChild(row);
		// append the <tbody> inside the <table>
		tbl.appendChild(tblBody);
		// put <table> in the <settingsControls>
		container.appendChild(tbl);	
	}
}

function setScreenWidth() {
	screenWidth = screen.width;
	if (screenWidth > 410) screenWidth = 410;
}

String.prototype.insertAt=function(charsToInsert,position){
	return this.slice(0,position) + charsToInsert + this.slice(position)
}

function loadSettingsControls() {
	console.log("**** loadSettingsControls ****")
	setScreenWidth();
	settingsColumn[0] = screenWidth - 100 + 'px';
	settingsColumn[1] = '30px';
	settingsColumn[2] = '30px';
 	var json = getJson();
	createSettingsTable();
	displayTime();
}

function getParams (url) {
	var params = {};
	var parser = document.createElement('a');
	parser.href = url;
	var query = parser.search.substring(1);
	var vars = query.split('&');
	for (var i = 0; i < vars.length; i++) {
		var pair = vars[i].split('=');
		params[pair[0]] = decodeURIComponent(pair[1]);
	}
	return params;
}

function loadPirLog() {
	console.log("**** loadPirControls ****");
	getJson();
	var params = getParams(window.location.href);
	console.log('params.log = ', params.log);
	var st = getFile(params.log);
//	console.log("st = ", st)
	if (st.includes('404:')) st = "Log File is Empty"; 
	else {
		st = st.replace(/B:/g,'');
		var dates = st.split('\n');
		st = "";
		for (var i=dates.length - 1; i>-1; i--) {
			if (dates[i].length > 5) st = st + timeString(dates[i],'true') + '</br>';
		}
		st = 'Motion Detected:</br>' + st;
	}
	document.getElementById("pir_log").innerHTML = st;
}

function clearLog() {
	console.log('**** Clear Log ******');
//	console.log ('Button.id = ' + button.id + ' mac = ' + button.getAttribute('mac'));
	var params = getParams(window.location.href);
	var ip = params.log.substring(params.log.indexOf('/log') + 4,params.log.indexOf('.txt'));
	for (i=10; i>0; i=i-2) ip = ip.insertAt(':',i);
	console.log ('ip = ',ip);

	devicejson = {};
	devicejson['mac'] = ip;
	devicejson['deleteLog'] = 99;	
	var st = JSON.stringify(devicejson);
	console.log("deleteLog json = " + st);
	connection.send('^' + st);
	location.reload();
}

var pirString = "";

function pirSettings(button) {
	console.log ('Button press...' + button.getAttribute('mac'));
	var st = '/log' + button.getAttribute('mac').replace(/:/g,'') + '.txt';
	console.log (st);
	window.location.href = '/pir.html?log=' + st;
}

function getDeviceColor(device){
	if (device.deviceType == DEVICE_GARAGE) {
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
	if (device.deviceType == DEVICE_LATCH) {
		if (device.online) {
			if (device.switchValue == 1) return RED; else return GREEN;
		} else {
			if (device.switchValue == 1) return OFFLINE_RED; else return OFFLINE_GREEN;
		}
	}
	if (device.deviceType == DEVICE_BAT) {
		if (device.online) {
			return GREEN;
		} else {
			return OFFLINE_GREEN;
		}
	}
	if (device.deviceType == DEVICE_CURTAIN) {
		if (device.online) {
			if (device.currentPosition <= 0) return GREEN; //Curtain is up
			else if (device.currentPosition > 900) return RED;
			else if (device.currentPosition >= device.rotationCount) return BLUE;
			else return YELLOW;
		} else {
			if (device.currentPosition <= 0) return OFFLINE_GREEN; //Curtain is up
			else if (device.currentPosition > 900) return OFFLINE_RED;
			else if (device.currentPosition >= device.rotationCount) return OFFLINE_BLUE;
			else return OFFLINE_YELLOW;		
		}
	}
}

function getIntervalJson() {
	console.log("getIntervalJson()");
	getJson();
}

function getFile(fileName) {
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET",fileName,false);
    Httpreq.send(null);
	return Httpreq.responseText;
}

function getJson(){
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);

	if (Httpreq.responseText !== "null") {
		processJson(Httpreq.responseText);
		json = JSON.parse(Httpreq.responseText)
		return json;
	}
	return;
}

/*
function get(url) {
	// Return a new promise.
	return new Promise(function(resolve, reject) {
	  // Do the usual XHR stuff
	  var req = new XMLHttpRequest();
	  req.open('GET', url);
  
	  req.onload = function() {
		// This is called even on 404 etc
		// so check the status
		if (req.status == 200) {
		  // Resolve the promise with the response text
		  resolve(req.response);
		}
		else {
		  // Otherwise reject with the status text
		  // which will hopefully be a meaningful error
		  reject(Error(req.statusText));
		}
	  };
  
	  // Handle network errors
	  req.onerror = function() {
		reject(Error("Network Error"));
	  };
  
	  // Make the request
	  req.send();
	});
  }
*/

function processJson(jsonString){

    if (jsonString !== "null") {
		json = JSON.parse(jsonString);

		timeOffset = json.global.timeOffset;
		var doc = document.getElementById('timeButton');
		if (doc !== null) doc.innerHTML = timeString(json.global.currTime);

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
//					var ip = json.devices[i].ip.split('.',4);
					button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + " " + deviceTime + "</p>";
				}
				if (json.devices[i].deviceType == DEVICE_THERMOMETER) {
					var t;

					if (json.devices[i].celcius == true) t = Math.round(json.devices[i].temp / 100); else t = (Math.round((json.devices[i].temp / 100) * 9/5) + 32);
					
					if ((t > json.devices[i].maxTemp) && (json.devices[i].maxTemp != 0)) button.style.backgroundColor = RED;
					else if ((t < json.devices[i].minTemp) && (json.devices[i].minTemp != 0)) button.style.backgroundColor = BLUE;
					else if (json.devices[i].online == 0) button.style.backgroundColor = YELLOW;
					else button.style.backgroundColor = GREEN;


					var st = "Last Read ";
					var deviceTime = timeString(json.devices[i].deviceTime);
					var degC = Math.round(json.devices[i].temp / 100) + "\xB0 C";
					var degF = Math.round( ((json.devices[i].temp / 100) * 9/5) + 32) + "\xB0 F";
					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + degF + ' / ' + degC + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + json.devices[i].deviceName + " @ " + deviceTime + "</p>";
				}
				if (json.devices[i].deviceType == DEVICE_LATCH) {		
					var deviceTime = timeString(json.devices[i].deviceTime);
					var latchTime = timeString(json.devices[i].latchTime);
					var st = "";
					if (json.devices[i].latchValue == 0) {
						button.style.backgroundColor = GREEN;
						st = "Closed since " + deviceTime;
					} else {
						button.style.backgroundColor = RED;
						st = "Opened " + latchTime + " (Currently ";
						if (json.devices[i].switchValue == 1) st += "open)"; else st += "closed)";
					}

					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + st + " " + deviceTime + "</p>";
				}
				if (json.devices[i].deviceType == DEVICE_BAT) {	
					button.style.backgroundColor = getDeviceColor(json.devices[i]);	
					var deviceTime = timeString(json.devices[i].deviceTime);
					var st = "Last Detection @ " + deviceTime;
					if (json.devices[i].alarmState == 0) st = "(OFF) " + st; else st = "(ON) " + st;
					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + st + "</p>";
				}			
				if (json.devices[i].deviceType == DEVICE_CURTAIN) {	
					button.style.backgroundColor = getDeviceColor(json.devices[i]);	
					var deviceTime = timeString(json.devices[i].deviceTime);
					if (json.devices[i].currentPosition <= 0) {
						var st = "Raised @ " + deviceTime;
					} else {
						var st = "Lowered @ " + deviceTime;
					}
					if (json.devices[i].online == 1) button.innerHTML = "<p class='pbutton1'>" + json.devices[i].deviceName + "</p></br><p class='pbutton2'>" + st + "</p>";
					else button.innerHTML = "<p class='pbutton1'>" + "OFFLINE" + "</p></br><p class='pbutton2'>" + st + "</p>";
				}			
			}
			var swapbutton = document.getElementById('swap' + json.devices[i].mac);
			if (swapbutton != null) {
				if (json.devices[i].deviceType === DEVICE_GARAGE) 
					if (json.devices[i].sensor0 === json.devices[i].sensor1) swapbutton.style.visibility = 'hidden';
					else swapbutton.style.visibility = 'visible';
				if (json.devices[i].deviceType === DEVICE_LATCH) swapbutton.style.visibility = 'visible';	
				swapbutton.style.backgroundColor = getDeviceColor(json.devices[i]);
			}
		}
	}
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

