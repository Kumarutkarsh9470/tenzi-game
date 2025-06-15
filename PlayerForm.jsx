import React, { useState } from "react";

const PlayerForm = ({ onStartGame }) => {
  const [name, setName] = useState("");

  const handleSubmit = (e) => {
    e.preventDefault();
    if (name.trim()) {
      onStartGame(name.trim());
    }
  };

  return (
    <div className="player-form">
      <h2>Enter Your Name to Start Playing</h2>
      <form onSubmit={handleSubmit}>
        <input
          type="text"
          placeholder="Your name..."
          value={name}
          onChange={(e) => setName(e.target.value)}
          className="name-input"
        />
        <button type="submit" className="start-btn">
          Start Game
        </button>
      </form>
    </div>
  );
};

export default PlayerForm;
