angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
           function ($scope, constellation)
           {
               $scope.editedBomb = JSON.parse(localStorage.getItem("editedBomb"));
               $scope.allBombComponents = $scope.editedBomb.Components;

               $scope.formatTime = formatTime;
               $scope.formattedTime = $scope.formatTime($scope.editedBomb.TimerInMs);
               $scope.$watch("formattedTime", function (value)
               {
                   var values = value.split(":");
                   var factor = 1000;
                   var result = 0;

                   for (var i = values.length - 1; i >= 0; --i)
                   {
                       result += factor * values[i];
                       factor *= 60;
                   }

                   $scope.editedBomb.TimerInMs = result;
               });
           }
       ]);

// Take a time in ms and return a string as hh:mm::ss.
function formatTime(timeInMs) {
    var seconds = Math.floor(timeInMs / 1000);
    var minutes = Math.floor(seconds / 60);
    seconds %= 60;

    var hours = Math.floor(minutes / 60);
    minutes %= 60;

    var strSeconds = ((seconds < 10) ? "0" : "") + seconds;
    var strMinutes = ((minutes < 10) ? "0" : "") + minutes;
    var strHours = ((hours < 10) ? "0" : "") + hours;

    var result = "" + strHours + ":" + strMinutes + ":" + strSeconds;

    return result;
}