import React from "react";

const Die = ({ value, isHeld, onHold }) => {
  return (
    <div className={`die ${isHeld ? "held" : ""}`} onClick={onHold}>
      {value}
    </div>
  );
};

export default Die;
