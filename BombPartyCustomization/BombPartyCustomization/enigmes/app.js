angular.module("mainApp", [])
       .controller("MainCtrl", ["$scope", function ($scope) {
           $scope.title = "enigme";
           $scope.enigmes = [
               {
                   "Number": "enigme1",
                   "link": "enigmes/enigme1/enigme1.html",
                   "Title": "65 536 = 0111",
                   "available": true,
                   "id": 0,
                   "Description": "",
                   "img": "",
                   "style": "",
               },
               {
                   "Number": "enigme2",
                   "link": "enigmes/enigme2/enigme2.html",
                   "Title": "ZERO",
                   "available": true,
                   "id": 1,
                   "Description": "Press Z E R O times on each button !",
                   "img": "img/Music.PNG",
                   "style": "width:304px;height:235px;"
               },
               {
                   "Number": "enigme3",
                   "link": "enigmes/enigme3/enigme3.html",
                   "Title": "Morse",
                   "available": true,
                   "id": 2,
                   "Description": "",
                   "img": "",
                   "style": "",
               },
               {
                   "Number": "enigme4",
                   "link": "enigmes/enigme4/enigme4.html",
                   "Title": "Emperor",
                   "available": true,
                   "id": 3,
                   "Description": "",
                   "img": "",
                   "style": "",
               },
               {
                   "Number": "enigme5",
                   "link": "enigmes/enigme5/enigme5.html",
                   "Title": "guess what?",
                   "available": true,
                   "id": 4,
                   "Description": "",
                   "img": "",
                   "style": "",
               },
               {
                   "Number": "enigme6",
                   "link": "enigmes/enigme6/enigme6.html",
                   "Title": "gray",
                   "available": true,
                   "id": 5,
                   "Description": "",
                   "img": "",
                   "style": "",
               },
               {
                   "Number": "enigme7",
                   "link": "enigmes/enigme7/enigme7.html",
                   "Title": "konami code",
                   "available": true,
                   "id": 6,
                   "Description": "",
                   "img": "",
                   "style": "",
               }
           ];
       }]);


//angular.module("mainApp", [])
//    .controller("MyController",
//				["$scope",
//				 function ($scope) {
//				     }
//                 ]);
