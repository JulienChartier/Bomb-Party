angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
            function ($scope, constellation) {
                $scope.connectionState = false;

                constellation.intializeClient("http://89.156.77.212:8088",
                                               "07284abc978bcdc9cb4713c5292d3900a54107b8", "BombPartyUI");

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
                        
                        switch (stateObject.Name)
                        {
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

                $scope.allBombs = {};
                $scope.selectedBomb = null;

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
                };

                $scope.pauseBomb = function(bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'PauseBomb', bomb.MacAddress);
                }

                $scope.resumeBomb = function (bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ResumeBomb', bomb.MacAddress);
                };

                $scope.resetBomb = function(bomb)
                {
                    constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ResetBomb', bomb.MacAddress);
                }

                $scope.editBomb = function(bomb)
                {
                    localStorage.setItem("editedBomb", JSON.stringify(bomb));
                    window.location = "/bomb_edition.html";
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
