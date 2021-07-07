const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <title>Gate Controller</title>
  <meta charset="utf-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <main>
    <h1>Gate Controller</h1><br>
    <div class="justify-content-center">
      <button type="button" class="btn btn-an" id="gatesignal" onclick="btnClick()">Abrir</button>
    </div>
    <h2 >Estado do portão: <u><span id="gatestate"></span></u></h2>
    <h2 id="error-msg"></h2>
  </main>
  <script type="text/javascript">
    var capture_interval = 250;
    function btnClick(button) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/gate_signal", true);
      xhr.send();

      button.setAttribute('disabled', true);
      setTimeout(function(){
          button.removeAttribute('disabled');
      }, 500)
    }
    
    setInterval(function ( ) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              if (this.responseText == "1" || this.responseText == "3") {
                document.getElementById("gatesignal").innerHTML = "Fechar";

              } else if (this.responseText == "4" || this.responseText == "6") {
                document.getElementById("gatesignal").innerHTML = "Abrir";

              } 
              else if (this.responseText == "2" || this.responseText == "5") {
                document.getElementById("gatesignal").innerHTML = "Parar";

              } else {
                document.getElementById("gatesignal").innerHTML = "Sinal";
              }
              
              elem = document.getElementById("gatestate")
              switch (this.responseText) {
                case "0":
                  elem.innerHTML = "Desconhecido";
                  break;

                case "1":
                case "3":
                case "6":
                  elem.innerHTML = "Aberto";
                  break;

                case "2":
                  elem.innerHTML = "A abrir";
                  break;

                case "4":
                  elem.innerHTML = "Fechado";
                  break;

                case "5":
                  elem.innerHTML = "A fechar";
                  break;
              }
            }
        };
        xhttp.open("GET", "/gatestate", true);
        xhttp.send();
    }, capture_interval ) ;


    setInterval(function ( ) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              if (this.responseText == "1"){
                document.getElementById("error-msg").innerHTML = "Ocorreu um erro com o portão!";
              } else {
                document.getElementById("error-msg").innerHTML = "";
              }
            }
        };
        xhttp.open("GET", "/gateerror", true);
        xhttp.send();
    }, capture_interval ) ;


  </script>
</body>
</html>
)rawliteral";