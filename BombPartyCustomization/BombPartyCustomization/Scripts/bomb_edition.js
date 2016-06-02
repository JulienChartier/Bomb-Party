angular.module("mainApp", ["ngConstellation"])
       .controller("MyController", ["$scope", "constellationConsumer",
           function ($scope, constellation)
           {
               $scope.editedBomb = JSON.parse(localStorage.getItem("editedBomb"));
               console.log($scope.editedBomb);
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
                       $scope.allInstructionComponentNames[i] = $scope.editedBomb.Instructions[i].Component.Name;
                       $scope.allInstructionComponentStates[i] = $scope.editedBomb.Instructions[i].Component.State;
                   }
               }

               $scope.init();

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
                       $scope.editedBomb.Instructions[i].Component.Name = $scope.allInstructionComponentNames[i];
                       $scope.editedBomb.Instructions[i].Component.State = $scope.allInstructionComponentStates[i];
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
                       return (instruction.Component.Name != "");
                   });

                   $scope.init();

                   localStorage.setItem("editedBomb", JSON.stringify($scope.editedBomb));
                   // constellation.sendMessage({ Scope: 'Package', Args: ['BombPartyServer'] }, 'ConfigureBomb', {
                   //                                                                                               MacAddress: editedBomb.MacAddress,
                   //                                                                                               Instructions: editedBomb.Instructions,
                   //                                                                                               Time: editedBomb.TimerInMs
                   //                                                                                             }); 
               }

               $scope.resetEditedBomb = function()
               {
                   $scope.editedBomb = JSON.parse(localStorage.getItem("editedBomb"));
                   $scope.init();
               }
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