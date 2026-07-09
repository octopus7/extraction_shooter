import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5178,
    strictPort: false
  },
  preview: {
    port: 4178,
    strictPort: false
  }
});
