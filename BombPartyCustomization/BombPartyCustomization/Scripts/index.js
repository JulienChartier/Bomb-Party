angular.module("mainApp", [])
    .controller("MyController",
				["$scope",
				 function ($scope)
				 {
					 $scope.login = function(asBomberman)
					 {
						 localStorage.setItem("isBomberman", asBomberman.toString());
						 window.location = "main.html";
					 }
				 }]);
