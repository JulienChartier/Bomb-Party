angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
           function ($scope, constellation)
           {
               $scope.connectionState = false;
               // NOTE: Find a way to pass constellation around?
               //constellation.intializeClient("http://89.156.77.212:8088",
               //                               "07284abc978bcdc9cb4713c5292d3900a54107b8", "BombPartyUI");
               constellation.intializeClient("http://localhost:8088",
                                               "630547c1fada14c61e876be55ac877e13f5c03d7", "BombPartyUI");

               constellation.onConnectionStateChanged(function (change) {
                   $scope.$apply(function () {
                       $scope.connectionState = (change.newState === $.signalR.connectionState.connected);
                   });

                   if ($scope.connectionState == true) {
                       //constellation.requestSubscribeStateObjects("*", "Bombi", "*", "*");
                   }
               });

               constellation.connect();

			   $scope.isBomberman = localStorage.getItem("isBomberman");
			   
               $scope.editedBomb = JSON.parse(localStorage.getItem("editedBomb"));

			   if ($scope.editedBomb == null)
			   {
				   return;
			   }
			   
               $scope.allBombComponents = $scope.editedBomb.Components;

               $scope.formatTime = formatTime;
               var formattedTime = $scope.formatTime($scope.editedBomb.TimeInMs);
               var values = formattedTime.split(":");

               $scope.formattedTime = { hours: parseInt(values[0]), minutes: parseInt(values[1]), seconds: parseInt(values[2]) };

               $scope.updateTime = function()
               {
                   var factor = 1000;
                   var result = 0;
                   var field = ["seconds", "minutes", "hours"];

                   for (var i in field)
                   {
                       var value = $scope.formattedTime[field[i]];

                       value = (value == undefined) ? 0 : value;
                       $scope.formattedTime[field[i]] = value;
                       
                       result += factor * value;
                       factor *= 60;
                   }

                   $scope.editedBomb.TimeInMs = result;
               };

               $scope.allInteractiveComponentNames = [];
               $scope.allDecorativeComponentNames = [];
               for (var i = 0; i < $scope.editedBomb.Components.length; ++i)
               {
                   if ($scope.editedBomb.Components[i].Type == "Button")
                   {
                       $scope.allInteractiveComponentNames.push($scope.editedBomb.Components[i].Name);
                   }
                   else
                   {
                       $scope.allDecorativeComponentNames.push($scope.editedBomb.Components[i].Name);
                   }
               }

               $scope.init = function () {
                   $scope.allInstructionComponentNames = [];
                   $scope.allInstructionComponentStates = [];

                   for (var i = 0; i < $scope.editedBomb.Instructions.length; ++i) {
                       $scope.allInstructionComponentNames[i] = $scope.editedBomb.Instructions[i].ComponentName;
                       $scope.allInstructionComponentStates[i] = $scope.editedBomb.Instructions[i].ComponentState;
                   }
               }

               $scope.checkMinDuration = function(instruction, value, oldValue)
               {
                   value = (value == undefined) ? 0 : value;
                   oldValue = (oldValue == undefined) ? 0 : oldValue;
                   instruction.MinDuration = (instruction.MinDuration == undefined) ? 0 : instruction.MinDuration;
                   instruction.MaxDuration = (instruction.MaxDuration == undefined) ? 0 : instruction.MaxDuration;

                   if ((instruction.MaxDuration != 0) && (instruction.MaxDuration < value))
                   {
                       instruction.MinDuration = instruction.MaxDuration;
                   }
               }

               $scope.checkMaxDuration = function(instruction, value, oldValue)
               {
                   value = (value == undefined) ? 0 : value;
                   oldValue = (oldValue == undefined) ? 0 : oldValue;
                   instruction.MinDuration = (instruction.MinDuration == undefined) ? 0 : instruction.MinDuration;
                   instruction.MaxDuration = (instruction.MaxDuration == undefined) ? 0 : instruction.MaxDuration;

                   if ((instruction.MinDuration != 0) && (instruction.MinDuration > value))
                   {
                       if (oldValue < value)
                       {
                           instruction.MaxDuration = instruction.MinDuration;
                       }
                       else
                       {
                           instruction.MaxDuration = 0;
                       }
                   }
               }

               $scope.invertState = function(state)
               {
                   switch(state)
                   {
                       case "Up":
                           {
                               return "Down";
                           }
                       default:
                           {
                               return "Up";
                           }
                   }
               }

               $scope.updateAllInstructionsAfterIndex = function (index, name)
               {
                   var oldName = $scope.allInstructionComponentNames[index];
                   var oldState = $scope.allInstructionComponentStates[index];

                   var lastIndex = $scope.allInstructionComponentNames.lastIndexOf(name, index);

                   var state = (lastIndex < 0) ? "Up" : $scope.allInstructionComponentStates[lastIndex];
                   $scope.allInstructionComponentNames[index] = name;
                   $scope.allInstructionComponentStates[index] = $scope.invertState(state);

                   state = $scope.invertState(state);

                   for (var i = index; i < $scope.allInstructionComponentNames.length; ++i)
                   {
                       if ($scope.allInstructionComponentNames[i] == oldName)
                       {
                           $scope.allInstructionComponentStates[i] = oldState;
                           oldState = $scope.invertState(oldState);
                       }
                       else if ($scope.allInstructionComponentNames[i] == name)
                       {
                           $scope.allInstructionComponentStates[i] = state;
                           state = $scope.invertState(state);
                       }
                   }

                   for (var i = 0; i < $scope.editedBomb.Instructions.length; ++i)
                   {
                       $scope.editedBomb.Instructions[i].ComponentName = $scope.allInstructionComponentNames[i];
                       $scope.editedBomb.Instructions[i].ComponentState = $scope.allInstructionComponentStates[i];
                   }
               };

               $scope.addNewInstruction = function(index)
               {
                   $scope.editedBomb.Instructions.splice(index, 0,
                       {
                           Component:
                           {
                               Name: "",
                               Type: "Button",
                               State: "Down"
                           },
                           MinDuration: 0,
                           MaxDuration: 0
                       });

                   $scope.allInstructionComponentNames.splice(index, 0, "");
                   $scope.allInstructionComponentStates.splice(index, 0, "Down");
               }

               $scope.removeInstruction = function(index)
               {
                   $scope.updateAllInstructionsAfterIndex(index, "");
                   $scope.allInstructionComponentNames.splice(index, 1);
                   $scope.allInstructionComponentStates.splice(index, 1);
                   $scope.editedBomb.Instructions.splice(index, 1);
               }

               $scope.saveEditedBomb = function()
               {
                   $scope.editedBomb.Instructions = $scope.editedBomb.Instructions.filter(function (instruction)
                   {
                       return (instruction.ComponentName != "");
                   });

                   $scope.init();

                   localStorage.setItem("editedBomb", JSON.stringify($scope.editedBomb));

                   constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ConfigureBomb',
                                             [$scope.editedBomb.MacAddress, $scope.editedBomb.TimeInMs, $scope.editedBomb.Instructions]);

                   window.location = "main.html";
               }

               $scope.resetEditedBomb = function()
               {
                   $scope.editedBomb = JSON.parse(localStorage.getItem("editedBomb"));
                   $scope.init();
               }

               $scope.init();
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
