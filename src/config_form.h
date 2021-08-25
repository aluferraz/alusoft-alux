const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {font-family: Arial, Helvetica, sans-serif;}
* {box-sizing: border-box;}

.form-inline {  
  display: flex;
  flex-flow: row wrap;
  align-items: center;
}

.form-inline label {
  margin: 5px 10px 5px 0;
}

.form-inline input {
  vertical-align: middle;
  margin: 5px 10px 5px 0;
  padding: 10px;
  background-color: #fff;
  border: 1px solid #ddd;
}

.form-inline button {
  padding: 10px 20px;
  background-color: dodgerblue;
  border: 1px solid #ddd;
  color: white;
  cursor: pointer;
}

.form-inline button:hover {
  background-color: royalblue;
}

@media (max-width: 800px) {
  .form-inline input {
    margin: 10px 0;
  }
  
  .form-inline {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
</head>
<body>

<h2>Configure sua <b>Alux</b></h2>

<form class="form-inline" action="/config_alux" method="post">
  <label for="email">Nome da sua Alux:</label>
  <input type="text" id="custom_alux_name" placeholder="Nome Alux" name="custom_alux_name">
  <label for="email">Nome da sua rede wifi:</label>
  <input type="text" id="custom_wifi_ssid" placeholder="Nome WiFi" name="custom_wifi_ssid">
  <label for="pwd">Senha da sua rede Wifi:</label>
  <input type="password" id="custom_wifi_pass" placeholder="Senha Wifi" name="custom_wifi_pass">
  <button type="submit">Configurar</button>
</form>

</body>
</html>

)=====";