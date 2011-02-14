$(document).ready(function() { 
    var tabs = $(".action-expand-tab").tabs({collapsible: true, selected: -1 });
    $(".ui-icon-circle-plus").live("click", function() { 
        var id = $(this).text();
        $("#"+id).trigger("mousedown");
    });
    $(".ui-icon-close").live("click", function() { 
        var id = $(this).text();
        $("#"+id).trigger("mousedown");
    });
    $("body").ajaxComplete(function(event, request, settings) {
        var tabs = $(".action-expand-tab").tabs({collapsible: true}); 
        var response = eval("(" + request.responseText + ")");
        jQuery.extend(Drupal.settings, response.settings);
        Drupal.attachBehaviors();
    });
});