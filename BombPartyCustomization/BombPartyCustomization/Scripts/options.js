angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
            function ($scope, constellation) {
                $scope.connectionState = false;

                //constellation.intializeClient("http://89.156.77.212:8088",
                //                               "07284abc978bcdc9cb4713c5292d3900a54107b8", "BombPartyUI");

                $scope.getConnectionSettings = function()
                {
                    $scope.serverUri = JSON.parse(localStorage.getItem("serverUri"));

                    if ($scope.serverUri == null)
                    {
                        $scope.serverUri = ["127", "0", "0", "1", "8088"];
                    }

                    $scope.accessKey = JSON.parse(localStorage.getItem("accessKey"));

                    if ($scope.accessKey == null)
                    {
                        $scope.accessKey = "630547c1fada14c61e876be55ac877e13f5c03d7";
                    }

                    $scope.newServerUri = $scope.serverUri.slice(0);
                    $scope.newAccessKey = $scope.accessKey.slice(0);
                }

                $scope.connectToConstellation = function()
                {
                    var uri = "http://";

                    for (var i = 0; i < 3; i++)
                    {
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
                            //constellation.requestSubscribeStateObjects("*", "Bombi", "*", "*");
                        }
                    });

                    constellation.connect();
                }

                $scope.updateConnectionSettings = function()
                {
                    localStorage.setItem("serverUri", JSON.stringify($scope.newServerUri));
                    localStorage.setItem("accessKey", JSON.stringify($scope.newAccessKey));

                    window.location = "options.html";
                }

                $scope.cancelConnectionChanges = function()
                {
                    $scope.newServerUri = $scope.serverUri.slice(0);
                    $scope.newAccessKey = $scope.accessKey.slice(0);
                }

                $scope.getConnectionSettings();
                $scope.connectToConstellation();
            }]);