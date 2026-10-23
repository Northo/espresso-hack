/* Progressive enhancement only: ordinary links and forms also work without JS. */
document.addEventListener("htmx:beforeSwap", function (event) {
  if (event.detail.xhr.status >= 400 && event.detail.xhr.status < 500) {
    event.detail.shouldSwap = true;
    event.detail.isError = false;
  }
});
document.addEventListener("htmx:beforeRequest", function () {
  document.getElementById("network-error").hidden = true;
});
["htmx:sendError", "htmx:responseError", "htmx:timeout", "htmx:swapError"].forEach(function (name) {
  document.addEventListener(name, function () {
    document.getElementById("network-error").hidden = false;
  });
});
