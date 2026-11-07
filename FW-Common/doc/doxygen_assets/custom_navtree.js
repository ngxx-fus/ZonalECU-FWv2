/**
 * @file custom_navtree.js
 * @brief Auto-expand Doxygen navigation treeview and persist expanded state across file clicks.
 */
(function() {
    function expandClosedNodes() {
        var closedArrows = document.querySelectorAll("#nav-tree .arrowhead.closed");
        closedArrows.forEach(function(arrow) {
            var toggle = arrow.closest("a");
            if (toggle) {
                toggle.click();
            }
        });

        // Highlight active link matching current URL
        var currentFile = window.location.pathname.split("/").pop();
        if (currentFile) {
            var links = document.querySelectorAll("#nav-tree a");
            links.forEach(function(link) {
                var href = link.getAttribute("href");
                if (href && (href === currentFile || href.indexOf(currentFile) !== -1)) {
                    var li = link.closest("li");
                    if (li) {
                        li.classList.add("selected");
                    }
                }
            });
        }
    }

    // Run on initial load with staggered retries for asynchronous navtree data
    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", function() {
            [100, 300, 600, 1000].forEach(function(delay) {
                setTimeout(expandClosedNodes, delay);
            });
        });
    } else {
        [100, 300, 600, 1000].forEach(function(delay) {
            setTimeout(expandClosedNodes, delay);
        });
    }
})();
