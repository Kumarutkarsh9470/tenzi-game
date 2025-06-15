import React, { useState } from "react";
import PlayerForm from "./components/PlayerForm";
import GameBoard from "./components/GameBoard";
import "./App.css";

function App() {
  const [playerName, setPlayerName] = useState("");
  const [gameStarted, setGameStarted] = useState(false);

  const handleStartGame = (name) => {
    setPlayerName(name);
    setGameStarted(true);
  };

  const handlePlayAgain = () => {
    setGameStarted(false);
    setPlayerName("");
  };

  return (
    <div className="App">
      <div className="game-container">
        <h1>Tenzies</h1>
        {!gameStarted ? (
          <PlayerForm onStartGame={handleStartGame} />
        ) : (
          <GameBoard playerName={playerName} onPlayAgain={handlePlayAgain} />
        )}
      </div>
    </div>
  );
}

export default App;
