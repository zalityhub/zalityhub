function ArmCollapsible(observer) {

  function click() {
    this.classList.toggle("active");
    const content = this.nextElementSibling;
    if (content.style.display === "block") {
      content.style.display = "none";
    } else {
      content.style.display = "block";
    }
    if (observer)
      observer(this);
  }

  const coll = document.getElementsByClassName("collapsible");
  for (let i = 0; i < coll.length; ++i)
    coll[i].addEventListener("click", click);
}
