import type { Metadata, Viewport } from "next";
import "./globals.css";

const siteUrl = "https://rt-mpbc.shahmdmahi.dpdns.org";

export const viewport: Viewport = {
  themeColor: "#00f2fe",
  colorScheme: "dark",
  width: "device-width",
  initialScale: 1,
};

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: {
    default: "RT-MPBC | Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver",
    template: "%s | RT-MPBC",
  },
  description: "A bare-metal, dependency-free multi-physics bond-graph compiler and symplectic Port-Hamiltonian geometric integrator executing in sub-50 nanoseconds on embedded control silicon with zero dynamic heap memory.",
  keywords: [
    "RT-MPBC",
    "bond-graph compiler",
    "symplectic solver",
    "port-hamiltonian",
    "real-time multi-physics",
    "embedded digital twin",
    "bare-metal C99",
    "Rust no_std",
    "pyrtmpbc",
    "implicit midpoint integrator",
    "karnopp scap",
    "passivity based observer",
    "switched dirac",
    "robotics control",
    "EV powertrain simulation"
  ],
  authors: [{ name: "Shah Md. Mahi", url: "https://github.com/shahmdmahi15" }],
  creator: "Shah Md. Mahi",
  publisher: "RT-MPBC Systems",
  alternates: {
    canonical: siteUrl,
  },
  openGraph: {
    title: "RT-MPBC: Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver",
    description: "Sub-50 nanosecond execution latency (>34M steps/sec), zero malloc, and strict thermodynamic energy preservation on bare-metal silicon.",
    url: siteUrl,
    siteName: "RT-MPBC",
    locale: "en_US",
    type: "website",
    images: [
      {
        url: "/og-banner.svg",
        width: 1200,
        height: 630,
        alt: "RT-MPBC Multi-Physics Bond-Graph Compiler Banner",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "RT-MPBC | Real-Time Multi-Physics Bond-Graph Compiler",
    description: "Sub-50ns execution latency, zero heap allocation, and exact symplectic energy preservation for embedded control loops.",
    images: ["/og-banner.svg"],
    creator: "@shahmdmahi",
  },
  icons: {
    icon: [
      { url: "/favicon.ico?v=2", sizes: "any" },
      { url: "/favicon-32x32.png?v=2", type: "image/png", sizes: "32x32" },
      { url: "/favicon-16x16.png?v=2", type: "image/png", sizes: "16x16" },
      { url: "/icon.svg?v=2", type: "image/svg+xml" }
    ],
    apple: [
      { url: "/apple-touch-icon.png?v=2", sizes: "180x180", type: "image/png" }
    ],
    shortcut: ["/favicon.ico?v=2"],
  },
  robots: {
    index: true,
    follow: true,
    googleBot: {
      index: true,
      follow: true,
      "max-video-preview": -1,
      "max-image-preview": "large",
      "max-snippet": -1,
    },
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  const jsonLd = {
    "@context": "https://schema.org",
    "@graph": [
      {
        "@type": "SoftwareApplication",
        "@id": `${siteUrl}/#software`,
        "name": "RT-MPBC",
        "alternateName": "Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver",
        "description": "Hard real-time multi-physics compiler and Port-Hamiltonian Dirac geometric integrator executing in sub-50 nanoseconds on embedded microcontrollers with zero dynamic memory allocation.",
        "url": siteUrl,
        "downloadUrl": "https://github.com/shahmdmahi15/rt-mpbc/releases/tag/v1.1.0",
        "applicationCategory": "DeveloperApplication, ScientificSoftware",
        "operatingSystem": "Embedded, Linux, Windows, macOS, ARM Cortex-M, DSP, RISC-V",
        "programmingLanguage": ["C99", "Rust", "Python"],
        "softwareVersion": "1.1.0",
        "license": "https://opensource.org/licenses/MIT",
        "offers": {
          "@type": "Offer",
          "price": "0.00",
          "priceCurrency": "USD"
        },
        "author": {
          "@type": "Person",
          "name": "Shah Md. Mahi",
          "url": "https://github.com/shahmdmahi15"
        }
      },
      {
        "@type": "WebSite",
        "@id": `${siteUrl}/#website`,
        "url": siteUrl,
        "name": "RT-MPBC Official Documentation",
        "description": "Comprehensive engineering documentation, API reference, and live symplectic simulator for RT-MPBC.",
        "inLanguage": "en-US"
      }
    ]
  };

  return (
    <html lang="en" className="dark scroll-smooth">
      <head>
        <link rel="icon" type="image/x-icon" href="/favicon.ico?v=3" />
        <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png?v=3" />
        <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png?v=3" />
        <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png?v=3" />
        <script
          type="application/ld+json"
          dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
        />
      </head>
      <body className="antialiased selection:bg-cyan-500/30 selection:text-cyan-200">
        {children}
      </body>
    </html>
  );
}
