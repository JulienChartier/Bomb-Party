angular.module("mainApp", [])
       .controller("MainCtrl", ["$scope", function ($scope) {
           $scope.title = "enigme";
           $scope.enigmes = enigmes;
       }]);


//angular.module("mainApp", [])
//    .controller("MyController",
//				["$scope",
//				 function ($scope) {
//				     }
//                 ]);
