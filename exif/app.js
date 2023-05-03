function loadInit() {
  function w(event) {
    const target = event.target;

    const h = target.offsetHeight;
    const w = target.offsetWidth;

    const x = Math.abs(event.offsetX);
    const y = Math.abs(event.offsetY);

    if (y < (h / 3.0))
      return 0;   // center

    const right = (w / 2.0);
    if (x < right)
      return -1;      // left
    return 1;       // right
  }

  const c = whereIsCursor(document);
}
