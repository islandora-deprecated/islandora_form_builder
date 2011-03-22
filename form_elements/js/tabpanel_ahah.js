formbuilder = {
    tabpanel: {
        loadPanels: function () {
            var load = '.action-load-tabpanel';
            var collapsible = '.property-collapsible-tabpanel';
            var collapsed = '.property-collapsed-tabpanel';
            var allTabs = $(load);
            var collapsibleTabs = allTabs.filter(collapsible);
            var nonCollapsibleTabs = allTabs.not(collapsible);
            var expandedTabs = collapsibleTabs.not(collapsed);
            var collapsedTabs = collapsibleTabs.filter(collapsed);
            collapsedTabs.tabs( {collapsible: true, selected: -1 });
            expandedTabs.tabs( {collapsible: true });
            nonCollapsibleTabs.tabs({});
        },
        enableActions: function () {
            $(".ui-icon-circle-plus").live("click", function() {
                var id = $(this).text();
                $("#"+id).trigger("mousedown");
            });
            $(".ui-icon-close").live("click", function() {
                var id = $(this).text();
                $("#"+id).trigger("mousedown");
            });
        }
    } 
};

$(document).ready(function() {
    formbuilder.tabpanel.loadPanels();
    formbuilder.tabpanel.enableActions();
    $("body").ajaxComplete(function(event, request, settings) {
        formbuilder.tabpanel.loadPanels();
        var response = eval("(" + request.responseText + ")");
        jQuery.extend(Drupal.settings, response.settings);
        Drupal.attachBehaviors();
    });
});