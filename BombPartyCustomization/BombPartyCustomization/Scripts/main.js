angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
            function ($scope, constellation) {
                $scope.connectionState = false;

                //constellation.intializeClient("http://89.156.77.212:8088",
                //                               "07284abc978bcdc9cb4713c5292d3900a54107b8", "BombPartyUI");
                $scope.getConnectionSettings = function () {
                    $scope.serverUri = JSON.parse(localStorage.getItem("serverUri"));

                    if ($scope.serverUri == null) {
                        $scope.serverUri = ["127", "0", "0", "1", "8088"];
                    }

                    $scope.accessKey = JSON.parse(localStorage.getItem("accessKey"));

                    if ($scope.accessKey == null) {
                        $scope.accessKey = "630547c1fada14c61e876be55ac877e13f5c03d7";
                    }

                    $scope.newServerUri = $scope.serverUri.slice(0);
                    $scope.newAccessKey = $scope.accessKey.slice(0);
                }

                $scope.connectToConstellation = function () {
                    var uri = "http://";

                    for (var i = 0; i < 3; i++) {
                        uri += $scope.serverUri[i];
                        uri += ".";
                    }

                    uri += $scope.serverUri[3] + ":" + $scope.serverUri[4];

                    constellation.intializeClient(uri, $scope.accessKey, "BombPartyUI");

                    constellation.onConnectionStateChanged(function (change) {
                        $scope.$apply(function () {
                            $scope.connectionState = (change.newState === $.signalR.connectionState.connected);
                        });

                        if ($scope.connectionState == true) {
                            constellation.requestSubscribeStateObjects("*", "Bombi", "*", "*");
                        }
                    });

                    constellation.onUpdateStateObject(function (stateObject) {
                        $scope.$apply(function () {
                            if ($scope[stateObject.PackageName] == undefined) {
                                $scope[stateObject.PackageName] = {};
                            }

                            $scope[stateObject.PackageName][stateObject.Name] = stateObject;

                            switch (stateObject.Name) {
                                case "BombInfo":
                                    {
                                        $scope.allBombs[stateObject.Value.MacAddress] = stateObject.Value;
                                        break;
                                    }
                                default:
                                    break;
                            }
                        });
                    });

                    constellation.connect();
                }

                $scope.getConnectionSettings();
                $scope.connectToConstellation();

                $scope.allBombs = {};
                $scope.selectedBomb = null;
				$scope.isBomberman = localStorage.getItem("isBomberman");

                // Unselect if already selected.
                $scope.selectBomb = function (bomb) {
                    if (($scope.selectedBomb == null) ||
                        ($scope.selectedBomb.MacAddress != bomb.MacAddress))
                    {
                        $scope.selectedBomb = bomb;
                    }
                    else
                    {
                        $scope.selectedBomb = null;
                    }
                };

                $scope.activateBomb = function(bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ActivateBomb', bomb.MacAddress);
					// Either
					//constellation.sendMessage({ Scope: 'Package', Args: ['Bombi'] }, 'ActivateBomb', bomb.MacAddress);
					// Or (and same for others)
					//constellation.sendMessage({ Scope: 'Sentinel', Args: ['ArduinoCard_' + bomb.MacAddress] }, 'ActivateBomb');
                };

                $scope.pauseBomb = function(bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'PauseBomb', bomb.MacAddress);
					//constellation.sendMessage({ Scope: 'Package', Args: ['Bombi'] }, 'PauseBomb', bomb.MacAddress);
                }

                $scope.resumeBomb = function (bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ResumeBomb', bomb.MacAddress);
					//constellation.sendMessage({ Scope: 'Package', Args: ['Bombi'] }, 'ResumeBomb', bomb.MacAddress);
                };

                $scope.resetBomb = function(bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ResetBomb', bomb.MacAddress);
					//constellation.sendMessage({ Scope: 'Package', Args: ['Bombi'] }, 'ResetBomb', bomb.MacAddress);
                }

                $scope.editBomb = function(bomb)
                {
                    localStorage.setItem("editedBomb", JSON.stringify(bomb));
                    window.location = "bomb_edition.html";
                }

                $scope.formatTime = formatTime;

                constellation.connect();
            }]);

// Take a time in ms and return a string as hh:mm::ss.
function formatTime(timeInMs) {
    var seconds = Math.floor(timeInMs / 1000);
    var minutes = Math.floor(seconds / 60);
    seconds %= 60;

    var hours = Math.floor(minutes / 60);
    minutes %= 60;

    var strSeconds = ((seconds < 10) ? "0" : "") + seconds;
    var strMinutes = ((minutes < 10) ? "0" : "") + minutes ;
    var strHours= ((hours < 10) ? "0" : "") + hours ;

    var result = "" + strHours + ":" + strMinutes + ":" + strSeconds;

    return result;
}
