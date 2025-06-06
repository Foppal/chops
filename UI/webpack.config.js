// webpack.config.js - Build configuration for Chops Browser UI
const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");

module.exports = (env, argv) => {
  const isProduction = argv.mode === "production";

  return {
    entry: "./index.js", // Changed from ChopsBrowserUI.jsx to index.js

    output: {
      path: path.resolve(__dirname, "dist"),
      filename: isProduction ? "chops-ui.[contenthash].js" : "chops-ui.js",
      clean: true,
      library: "ChopsBrowserUI",
      libraryTarget: "umd",
      globalObject: "this",
    },

    resolve: {
      extensions: [".js", ".jsx"],
      alias: {
        "@": path.resolve(__dirname, "."),
        "@components": path.resolve(__dirname, "components"),
        "@utils": path.resolve(__dirname, "utils"),
        "@styles": path.resolve(__dirname, "styles"),
      },
    },

    module: {
      rules: [
        {
          test: /\.(js|jsx)$/,
          exclude: /node_modules/,
          use: {
            loader: "babel-loader",
            options: {
              presets: ["@babel/preset-env", "@babel/preset-react"],
            },
          },
        },
        {
          test: /\.css$/,
          use: ["style-loader", "css-loader"],
        },
        {
          test: /\.(png|svg|jpg|jpeg|gif|ico)$/,
          type: "asset/resource",
        },
        {
          test: /\.(woff|woff2|eot|ttf|otf)$/,
          type: "asset/resource",
        },
      ],
    },

    plugins: [
      new HtmlWebpackPlugin({
        template: "./index.html",
        filename: "index.html",
        inject: true,
        minify: isProduction
          ? {
              removeComments: true,
              collapseWhitespace: true,
              removeRedundantAttributes: true,
              useShortDoctype: true,
              removeEmptyAttributes: true,
              removeStyleLinkTypeAttributes: true,
              keepClosingSlash: true,
              minifyJS: true,
              minifyCSS: true,
              minifyURLs: true,
            }
          : false,
      }),
    ],

    devServer: {
      static: {
        directory: path.join(__dirname, "dist"),
      },
      compress: true,
      port: 3001,
      hot: true,
      open: true,
      historyApiFallback: true,
    },

    optimization: {
      splitChunks: isProduction
        ? {
            chunks: "all",
            cacheGroups: {
              vendor: {
                test: /[\\/]node_modules[\\/]/,
                name: "vendors",
                chunks: "all",
              },
            },
          }
        : false,
      minimize: isProduction,
    },

    devtool: isProduction ? "source-map" : "eval-source-map",

    externals: {
      // If using JUCE WebBrowserComponent, you might want to externalize React
      // and provide it globally from the C++ side
      // 'react': 'React',
      // 'react-dom': 'ReactDOM'
    },

    performance: {
      hints: isProduction ? "warning" : false,
      maxEntrypointSize: 512000,
      maxAssetSize: 512000,
    },
  };
};
