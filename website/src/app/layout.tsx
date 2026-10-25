import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "RT-MPBC | Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver",
  description: "A bare-metal, dependency-free multi-physics compiler and geometric integrator that bridges the gap between offline differential simulators and microsecond embedded control loops.",
  keywords: ["multi-physics", "bond-graphs", "symplectic-solver", "port-hamiltonian", "bare-metal", "embedded-systems", "robotics", "real-time"],
  authors: [{ name: "Shah Md. Mahi", url: "https://github.com/shahmdmahi15" }],
  openGraph: {
    title: "RT-MPBC: Real-Time Multi-Physics Bond-Graph Compiler",
    description: "Sub-50 nanosecond real-time Port-Hamiltonian symplectic solver with zero dynamic memory allocation.",
    url: "https://rt-mpbc.shahmdmahi.dpdns.org",
    siteName: "RT-MPBC Documentation",
    type: "website",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark scroll-smooth">
      <body className="antialiased selection:bg-cyan-500/30 selection:text-cyan-200">
        {children}
      </body>
    </html>
  );
}
