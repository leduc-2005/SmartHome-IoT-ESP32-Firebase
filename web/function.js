var database = firebase.database();

var nhietDoElement = document.getElementById("nhietdo");
var doAmElement = document.getElementById("doam");
var khiGasElement = document.getElementById("khigas");
var chuyenDongElement = document.getElementById("chuyendong");
var canhBaoElement = document.getElementById("canhbao");
var ledStatusElement = document.getElementById("led-status");

// Biến điều khiển Icon
var iconChuyenDong = document.getElementById("icon-chuyendong");
var iconDen = document.getElementById("icon-den");
var iconKhiGas = document.getElementById("icon-khigas");
var iconCanhBao = document.getElementById("icon-canhbao"); // Biến Icon Cảnh báo mới thêm

var btnLedOn = document.getElementById("btn-led-on");
var btnLedOff = document.getElementById("btn-led-off");

function updateClock() {
    var now = new Date();

    var gio = now.getHours();
    var phut = now.getMinutes();
    var giay = now.getSeconds();

    if (gio < 10) gio = "0" + gio;
    if (phut < 10) phut = "0" + phut;
    if (giay < 10) giay = "0" + giay;

    document.getElementById("time").innerText = gio + ":" + phut + ":" + giay;
}

setInterval(updateClock, 1000);
updateClock();

function showValue(value, unit) {
    if (value === null || value === undefined || value === "") {
        return "-- " + unit;
    }
    return value + " " + unit;
}

database.ref("quan1/NhietDo").on("value", function(snapshot) {
    var value = snapshot.val();
    nhietDoElement.innerText = showValue(value, "°C");
});

database.ref("quan1/DoAm").on("value", function(snapshot) {
    var value = snapshot.val();
    doAmElement.innerText = showValue(value, "%");
});

database.ref("quan1/KhiGas").on("value", function(snapshot) {
    var value = snapshot.val();
    if (value === null || value === undefined || value === "") {
        khiGasElement.innerText = "--";
    } else {
        khiGasElement.innerText = value;
    }
});

database.ref("quan1/ChuyenDong").on("value", function(snapshot) {
    var value = Number(snapshot.val());

    if (value === 1) {
        chuyenDongElement.innerText = "Có";
        chuyenDongElement.className = "warning";
        if (iconChuyenDong) iconChuyenDong.src = "cochuyendong.png"; 
    } else {
        chuyenDongElement.innerText = "Không";
        chuyenDongElement.className = "normal";
        if (iconChuyenDong) iconChuyenDong.src = "khongchuyendong.png"; 
    }
});

database.ref("quan1/CanhBao").on("value", function(snapshot) {
    var value = Number(snapshot.val());

    if (value === 1) {
        canhBaoElement.innerText = "Có cảnh báo";
        canhBaoElement.className = "warning";
        if (iconCanhBao) iconCanhBao.src = "canhbaoon.png"; // Bật icon cảnh báo
        if (iconKhiGas) iconKhiGas.src = "gason.png"; // Bật icon khí gas
    } else {
        canhBaoElement.innerText = "Bình thường";
        canhBaoElement.className = "normal";
        if (iconCanhBao) iconCanhBao.src = "canhbaooff.png"; // Tắt icon cảnh báo
        if (iconKhiGas) iconKhiGas.src = "gasoff.png"; // Tắt icon khí gas
    }
});

database.ref("thietbi3/den").on("value", function(snapshot) {
    var value = Number(snapshot.val());

    if (value === 1) {
        ledStatusElement.innerText = "Đang bật";
        ledStatusElement.className = "normal";
        if (iconDen) iconDen.src = "denon1.gif"; 
    } else {
        ledStatusElement.innerText = "Đang tắt";
        ledStatusElement.className = "warning";
        if (iconDen) iconDen.src = "den1.png"; 
    }
});

btnLedOn.addEventListener("click", function() {
    database.ref("control/den").set(1);
});

btnLedOff.addEventListener("click", function() {
    database.ref("control/den").set(0);
});
